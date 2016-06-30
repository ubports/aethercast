/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <boost/concept_check.hpp>

#include <algorithm>
#include <sstream>

#include <ac/logger.h>
#include <ac/keep_alive.h>
#include <ac/networkutils.h>

#include "w11tng/networkmanager.h"
#include "w11tng/informationelement.h"
#include "w11tng/kernelrfkillmanager.h"
#include "w11tng/urfkillmanager.h"

namespace {
// We take two minutes as timeout here which corresponds to what wpa
// takes for the group formation.
static const std::chrono::seconds kConnectTimeout{120};
static constexpr std::int32_t kSourceGoIntent = 13;
}

namespace w11tng {

ac::NetworkManager::Ptr NetworkManager::Create() {
    return std::shared_ptr<NetworkManager>(new NetworkManager())->FinalizeConstruction();
}

std::shared_ptr<NetworkManager> NetworkManager::FinalizeConstruction() {
    auto sp = shared_from_this();

    GError *error = nullptr;
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!connection_) {
        AC_ERROR("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    // We first check if urfkilld is running or not. If its not we fall back
    // to just use the plain rfkill interface the kernel offers.
    AC_DEBUG("Checking if URfkill is available ..");
    urfkill_watch_ = g_bus_watch_name(G_BUS_TYPE_SYSTEM,
          URfkillManager::kBusName,
          G_BUS_NAME_WATCHER_FLAGS_NONE,
          &NetworkManager::OnURfkillAvailable,
          &NetworkManager::OnURfkillNotAvailable,
          new ac::WeakKeepAlive<NetworkManager>(shared_from_this()),
          [](gpointer data) { delete static_cast<ac::WeakKeepAlive<NetworkManager>*>(data); });

    return sp;
}

NetworkManager::NetworkManager() :
    firmware_loader_("", this),
    dedicated_p2p_interface_(ac::Utils::GetEnvValue("AETHERCAST_DEDICATED_P2P_INTERFACE")),
    session_available_(true),
    urfkill_watch_(0) {
}

NetworkManager::~NetworkManager() {
}

void NetworkManager::OnServiceFound(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data) {
    boost::ignore_unused_variable_warning(connection);
    boost::ignore_unused_variable_warning(name);
    boost::ignore_unused_variable_warning(name_owner);

    auto inst = static_cast<ac::WeakKeepAlive<NetworkManager>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    inst->Initialize(true);
}

void NetworkManager::OnServiceLost(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    boost::ignore_unused_variable_warning(connection);
    boost::ignore_unused_variable_warning(name);

    auto inst = static_cast<ac::WeakKeepAlive<NetworkManager>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    inst->ReleaseInternal();
}

void NetworkManager::Initialize(bool firmware_loading) {
    if (firmware_loading && ac::Utils::GetEnvValue("AETHERCAST_NEED_FIRMWARE") == "1") {
        auto interface_name = ac::Utils::GetEnvValue("AETHERCAST_DEDICATED_P2P_INTERFACE");
        if (interface_name.empty())
            interface_name = "p2p0";

        AC_DEBUG("Firmware loading for interface '%s' requested", interface_name);

        firmware_loader_.SetInterfaceName(interface_name);
        if (firmware_loader_.IsNeeded()) {
            AC_DEBUG("Loading WiFi firmware for interface %s", interface_name);
            firmware_loader_.TryLoad();
            return;
        }
    }

    auto sp = shared_from_this();

    hostname_service_ = Hostname1Stub::Create(sp);

    interface_selector_ = InterfaceSelector::Create();
    interface_selector_->SetDelegate(sp);

    manager_ = ManagerStub::Create();
    manager_->SetDelegate(sp);

    AC_DEBUG("Successfully initialized");
}

void NetworkManager::Release() {
    AC_DEBUG("");

    for (auto &iter : devices_) {
        if (delegate_)
            delegate_->OnDeviceLost(iter.second);
    }

    devices_.clear();

    ReleaseInternal();
}

void NetworkManager::ReleaseInternal() {
    ReleaseInterface();

    hostname_service_.reset();
    interface_selector_.reset();
    manager_.reset();
}

void NetworkManager::SetupInterface(const std::string &object_path) {
    if (p2p_device_)
        return;

    mgmt_interface_ = InterfaceStub::Create(object_path);
    mgmt_interface_->SetDelegate(shared_from_this());

    p2p_device_ = P2PDeviceStub::Create(object_path, shared_from_this());

    ConfigureFromCapabilities();
}

void NetworkManager::ReleaseInterface() {
    AC_DEBUG("");

    if (current_device_) {
        AdvanceDeviceState(current_device_, ac::kDisconnected);
        current_device_.reset();
        current_group_device_.reset();
        current_group_iface_.reset();
    }

    if (p2p_device_)
        p2p_device_.reset();

    if (mgmt_interface_)
        mgmt_interface_.reset();
}

void NetworkManager::SetDelegate(ac::NetworkManager::Delegate *delegate) {
    delegate_ = delegate;
}

bool NetworkManager::Setup() {
    if (!Ready())
        return false;

    g_bus_watch_name_on_connection(connection_.get(),
                                   kBusName,
                                   G_BUS_NAME_WATCHER_FLAGS_NONE,
                                   &NetworkManager::OnServiceFound,
                                   &NetworkManager::OnServiceLost,
                                   new ac::WeakKeepAlive<NetworkManager>(shared_from_this()),
                                   nullptr);

    return true;
}

void NetworkManager::OnURfkillAvailable(GDBusConnection*, const gchar*, const gchar*, gpointer user_data) {
    auto inst = static_cast<ac::WeakKeepAlive<NetworkManager>*>(user_data)->GetInstance().lock();
    if (!inst)
        return;

    AC_DEBUG("URfkill is available");

    inst->rfkill_manager_ = URfkillManager::Create();
    inst->FinishRfkillInitialization();
}

void NetworkManager::OnURfkillNotAvailable(GDBusConnection*, const gchar*, gpointer user_data) {
    auto inst = static_cast<ac::WeakKeepAlive<NetworkManager>*>(user_data)->GetInstance().lock();
    if (!inst)
        return;

    AC_DEBUG("URfkill is not available, falling back to kernel rfkill manager");

    inst->rfkill_manager_ = KernelRfkillManager::Create();
    inst->FinishRfkillInitialization();
}

void NetworkManager::FinishRfkillInitialization() {
    g_source_remove(urfkill_watch_);
    urfkill_watch_ = 0;

    rfkill_manager_->SetDelegate(shared_from_this());
}

void NetworkManager::Scan(const std::chrono::seconds &timeout) {
    if (!p2p_device_)
        return;

    p2p_device_->Find(timeout);
}

NetworkDevice::Ptr NetworkManager::FindDevice(const std::string &address) {
    for (auto iter : devices_) {
        if (iter.second->Address() == address)
            return iter.second;
    }
    return NetworkDevice::Ptr{};
}

void NetworkManager::StartConnectTimeout() {
    AC_DEBUG("");

    connect_timeout_ = g_timeout_add_seconds(kConnectTimeout.count(), [](gpointer user_data) {
        auto inst = static_cast<ac::SharedKeepAlive<NetworkManager>*>(user_data)->ShouldDie();
        if (not inst)
            return FALSE;

        AC_WARNING("Reached a timeout while trying to connect with remote %s", inst->current_device_->Address());

        inst->connect_timeout_ = 0;

        // If the device is either already connected or trying to get an address
        // over DHCP we don't do anything. DHCP process will fail on its own after
        // some time and we will react on
        if (inst->current_device_->State() == ac::kConnected ||
            inst->current_device_->State() == ac::kConfiguration)
            return FALSE;

        inst->p2p_device_->Cancel();

        inst->AdvanceDeviceState(inst->current_device_, ac::kFailure);

        inst->current_device_.reset();

        // We don't have an active group if we're not in connected or configuration
        // state so we don't have to care about terminating any group at this point.

        return FALSE;
    }, new ac::SharedKeepAlive<NetworkManager>{shared_from_this()});
}

void NetworkManager::StopConnectTimeout() {
    if (connect_timeout_ == 0)
        return;

    AC_DEBUG("");

    g_source_remove(connect_timeout_);
    connect_timeout_ = 0;
}

bool NetworkManager::Connect(const ac::NetworkDevice::Ptr &device) {
    if (!p2p_device_ || current_device_)
        return false;

    if (!device)
        return false;

    AC_DEBUG("address %s", device->Address());

    // Lets check here if we really own this device and if yes then we
    // select our own instance of it rather than relying on the input
    auto d = FindDevice(device->Address());
    if (!d) {
        AC_WARNING("Could not find instance for device %s", device->Address());
        return false;
    }

    current_device_ = d;

    p2p_device_->StopFind();

    if (!p2p_device_->Connect(d->ObjectPath(), kSourceGoIntent))
        return false;

    current_device_->SetState(ac::kAssociation);
    if (delegate_)
        delegate_->OnDeviceStateChanged(current_device_);

    StartConnectTimeout();

    return true;
}

std::string NetworkManager::SelectHostname() {
    auto hostname = hostname_service_->PrettyHostname();
    if (hostname.length() == 0)
        hostname = hostname_service_->StaticHostname();
    if (hostname.length() == 0)
        hostname = hostname_service_->Hostname();
    if (hostname.length() == 0) {
        // Our last resort is to get the hostname via a system
        // call and not from the hostname service.
        char name[HOST_NAME_MAX + 1] = {};
        ::gethostname(name, HOST_NAME_MAX);
        hostname = name;
    }
    return hostname;
}

std::string NetworkManager::SelectDeviceType() {
    std::string oui = "0050F204";
    std::string category = "0001";
    std::string sub_category = "0000";

    auto chassis = hostname_service_->Chassis();
    if (chassis == "handset") {
        category = "000A";
        sub_category = "0005";
    }
    else if (chassis == "vm" || chassis == "container")
        sub_category = "0001";
    else if (chassis == "server")
        sub_category = "0002";
    else if (chassis == "laptop")
        sub_category = "0005";
    else if (chassis == "desktop")
        sub_category = "0006";
    else if (chassis == "tablet")
        sub_category = "0009";
    else if (chassis == "watch")
        sub_category = "00FF";

    return ac::Utils::Sprintf("%s%s%s", category, oui, sub_category);
}

void NetworkManager::SyncDeviceConfiguration() {
    if (!p2p_device_)
        return;

    auto hostname = SelectHostname();
    auto device_type = SelectDeviceType();

    p2p_device_->SetDeviceConfiguration(hostname, device_type);
}

bool NetworkManager::Disconnect(const ac::NetworkDevice::Ptr &device) {
    if (!p2p_device_ || !current_device_)
        return false;

    if (!FindDevice(device->Address()))
        return false;

    if (current_group_device_) {
        // This will trigger the GroupFinished signal where we will release
        // all parts in order.
        current_group_device_->Disconnect();
    }

    return true;
}

std::vector<ac::NetworkDevice::Ptr> NetworkManager::Devices() const {
    std::vector<ac::NetworkDevice::Ptr> values;
    std::transform(devices_.begin(), devices_.end(),
                   std::back_inserter(values),
                   [=](const std::pair<std::string,w11tng::NetworkDevice::Ptr> &value) {
        return value.second;
    });
    return values;
}

ac::IpV4Address NetworkManager::LocalAddress() const {
    ac::IpV4Address address;

    if (dhcp_server_)
        address = dhcp_server_->LocalAddress();
    else if (dhcp_client_)
        address = dhcp_client_->LocalAddress();

    AC_DEBUG("address %s", address);

    return address;
}

bool NetworkManager::Running() const {
    return p2p_device_ && p2p_device_->Connected();
}

bool NetworkManager::Scanning() const {
    return p2p_device_ && p2p_device_->Scanning();
}

bool NetworkManager::Ready() const {
    if (!rfkill_manager_)
        return false;

    return !rfkill_manager_->IsBlocked(RfkillManager::Type::kWLAN);
}

void NetworkManager::OnP2PDeviceChanged() {
    if (delegate_)
        delegate_->OnChanged();
}

void NetworkManager::OnP2PDeviceReady() {
    AC_DEBUG("");
    // Bring the device into a well known state
    p2p_device_->Flush();
    SyncDeviceConfiguration();
}

void NetworkManager::OnDeviceFound(const std::string &path) {
    if (devices_.find(path) != devices_.end())
        return;

    auto device = NetworkDevice::Create(path);
    device->SetDelegate(shared_from_this());
    devices_[path] = device;

    // NOTE: OnDeviceFound will be send to delegate once the device
    // reports through OnDeviceReady that its ready for operation.
}

void NetworkManager::OnDeviceLost(const std::string &path) {
    if (devices_.find(path) == devices_.end())
        return;

    AC_DEBUG("peer %s", path);

    auto device = devices_[path];
    devices_.erase(path);

    // If we're currently connecting with the lost device (which can
    // happen if we're the owner of the group and the remote disappears)
    // then we have to disconnect everything too.
    if (current_device_ == device && current_group_device_)
        current_group_device_->Disconnect();

    if (delegate_)
        delegate_->OnDeviceLost(device);
}

void NetworkManager::AdvanceDeviceState(const NetworkDevice::Ptr &device, ac::NetworkDeviceState state) {
    device->SetState(state);

    if (state == ac::kDisconnected) {
        if (ac::NetworkUtils::SendDriverPrivateCommand(mgmt_interface_->Ifname(),
                                                        BuildMiracastModeCommand(MiracastMode::kOff)) < 0)
            AC_DEBUG("Failed to disable WiFi driver miracast mode (not supported?)");
        else
            AC_DEBUG("Disabled WiFi driver miracast mode");
    }

    // When we're switching to be connected or disconnected we need to mark the
    // session as not being available to prevent anyone else to connect with us.
    if (state == ac::kConnected || state == ac::kDisconnected) {
        session_available_ = (state != ac::kConnected);
        ConfigureFromCapabilities();
    }

    if (delegate_)
        delegate_->OnDeviceStateChanged(device);
}

void NetworkManager::HandleConnectFailed() {
    AdvanceDeviceState(current_device_, ac::kFailure);
    current_device_.reset();
    StopConnectTimeout();
}

void NetworkManager::OnPeerConnectFailed() {
    if (!current_device_)
        return;

    AC_DEBUG("");

    HandleConnectFailed();
}

void NetworkManager::OnGroupOwnerNegotiationFailure(const std::string &peer_path, const P2PDeviceStub::GroupOwnerNegotiationResult &result) {
    if (!current_device_)
        return;

    AC_DEBUG("Connecting with peer %s failed: %s", peer_path, P2PDeviceStub::StatusToString(result.status));

    HandleConnectFailed();
}

void NetworkManager::OnGroupOwnerNegotiationSuccess(const std::string &peer_path, const P2PDeviceStub::GroupOwnerNegotiationResult &result) {
    if (!current_device_)
        return;

    std::stringstream frequencies;
    int n = 0;
    for (auto freq : result.frequencies) {
        frequencies << std::to_string(freq);
        if (n < result.frequencies.size() - 1)
            frequencies << ",";
        n++;
    }

    AC_DEBUG("peer %s selected oper freq %d wps_method %s",
              peer_path, result.oper_freq, P2PDeviceStub::WpsMethodToString(result.wps_method));
    AC_DEBUG("intersect freqs [%s]", frequencies.str());
}

void NetworkManager::OnGroupStarted(const std::string &group_path, const std::string &interface_path, const std::string &role) {
    if (!current_device_)
        return;

    AC_DEBUG("group %s interface %s role %s", group_path, interface_path, role);

    AdvanceDeviceState(current_device_, ac::kConfiguration);

    current_device_->SetRole(role);

    // We have to find out more about the actual group we're now part of
    // and which role we play in it.
    current_group_iface_ = InterfaceStub::Create(interface_path);
    current_group_iface_->SetDelegate(shared_from_this());

    std::weak_ptr<P2PDeviceStub::Delegate> null_delegate;
    current_group_device_ = P2PDeviceStub::Create(interface_path, null_delegate);
}

void NetworkManager::OnGroupFinished(const std::string &group_path, const std::string &interface_path) {
    if (!current_device_)
        return;

    AC_DEBUG("group %s interface %s", group_path, interface_path);

    StopConnectTimeout();

    dhcp_client_.reset();
    dhcp_server_.reset();

    current_group_iface_.reset();
    current_group_device_.reset();

    AdvanceDeviceState(current_device_, ac::kDisconnected);
    current_device_.reset();
}

void NetworkManager::OnGroupRequest(const std::string &peer_path, int dev_passwd_id) {
    AC_DEBUG("peer %s dev_passwd_id %d", peer_path, dev_passwd_id);

    // FIXME once we implement sink support we need to have this
    // respected as well
}

void NetworkManager::OnDeviceChanged(const NetworkDevice::Ptr &device) {
    if (delegate_)
        delegate_->OnDeviceChanged(device);
}

void NetworkManager::OnDeviceReady(const NetworkDevice::Ptr &device) {
    if (delegate_)
        delegate_->OnDeviceFound(device);
}

void NetworkManager::OnDhcpAddressAssigned(const ac::IpV4Address &local_address, const ac::IpV4Address &remote_address) {
    if (!current_device_ || current_device_->State() != ac::kConfiguration)
        return;

    AC_DEBUG("local %s remote %s", local_address, remote_address);

    current_device_->SetIpV4Address(remote_address);

    StopConnectTimeout();

    AdvanceDeviceState(current_device_, ac::kConnected);
}

void NetworkManager::OnDhcpTerminated() {
    if (!current_device_ || current_device_->State() != ac::kConfiguration)
        return;

    AC_DEBUG("");

    Disconnect(current_device_);
    AdvanceDeviceState(current_device_, ac::kFailure);
}

void NetworkManager::OnFirmwareLoaded() {
    // Pass through when firmware was successfully loaded and
    // do all other needed initialization stuff
    Initialize();
}

void NetworkManager::OnFirmwareUnloaded() {
}

void NetworkManager::OnInterfaceSelectionDone(const std::string &path) {
    if (path.length() == 0)
        return;

    AC_DEBUG("Found P2P interface %s", path);
    SetupInterface(path);
}

void NetworkManager::SetCapabilities(const std::vector<Capability> &capabilities) {
    if (capabilities == capabilities_)
        return;

    capabilities_ = capabilities;
    ConfigureFromCapabilities();
}

std::vector<NetworkManager::Capability> NetworkManager::Capabilities() const {
    return capabilities_;
}

DeviceType NetworkManager::GenerateWfdDeviceType() {
    DeviceType device_type = DeviceType::kSource;
    bool has_source = false, has_sink = false;

    for (auto capability : capabilities_) {
        if (capability == Capability::kSource)
            has_source = true;
        else if (capability == Capability::kSink)
            has_sink = true;
    }

    if (has_sink && !has_source)
        device_type = DeviceType::kPrimarySink;
    else if (!has_sink && has_source)
        device_type = DeviceType::kSource;
    else if (has_sink && has_source)
        device_type = DeviceType::kDualRole;

    return device_type;
}

void NetworkManager::ConfigureFromCapabilities() {
    if (!manager_)
        return;

    InformationElement ie;
    auto sub_element = new_subelement(kDeviceInformation);
    auto dev_info = (DeviceInformationSubelement*) sub_element;

    auto device_type = GenerateWfdDeviceType();

    AC_DEBUG("device type %d session availability %d",
              device_type,
              session_available_);

    dev_info->session_management_control_port = htons(7236);
    dev_info->maximum_throughput = htons(50);
    dev_info->field1.device_type = device_type;
    dev_info->field1.session_availability = session_available_;
    ie.add_subelement(sub_element);

    auto ie_data = ie.serialize();

    manager_->SetWFDIEs(ie_data->bytes, ie_data->length);
}

void NetworkManager::OnManagerReady() {
    // If we need to create an interface object at wpa first we
    // do that and continue in one of the delegate callbacks from
    // the manager stub.
    if (dedicated_p2p_interface_.length() > 0) {
        manager_->CreateInterface(dedicated_p2p_interface_);
        return;
    }

    interface_selector_->Process(manager_->Interfaces());
}

void NetworkManager::OnManagerInterfaceAdded(const std::string &path) {
    if (p2p_device_)
        return;

    interface_selector_->Process(manager_->Interfaces());
}

void NetworkManager::OnManagerInterfaceRemoved(const std::string &path) {
    AC_DEBUG("path %s", path);

    if (!p2p_device_)
        return;

    if (p2p_device_->ObjectPath() != path)
        return;

    ReleaseInterface();
}

void NetworkManager::OnManagerInterfaceCreationFailed() {
    // When interface creation failed its most likely that we were
    // restarted and that the interface stayed available at wpa and
    // we can simply start and reuse it here.
    interface_selector_->Process(manager_->Interfaces());
}

void NetworkManager::OnInterfaceReady(const std::string &object_path) {
    if (current_group_iface_ && object_path == current_group_iface_->ObjectPath())
        OnGroupInterfaceReady();
    else if (object_path == mgmt_interface_->ObjectPath())
        OnManagementInterfaceReady();
}

void NetworkManager::OnManagementInterfaceReady() {
}

std::string NetworkManager::BuildMiracastModeCommand(MiracastMode mode) {
    return ac::Utils::Sprintf("MIRACAST %d", static_cast<int>(mode));
}

void NetworkManager::OnGroupInterfaceReady() {
    if (!current_device_ || current_device_->State() != ac::kConfiguration)
        return;

    auto ifname = current_group_iface_->Ifname();

    // Android WiFi drivers have a special mode build in when they should
    // perform well for Miracast which we enable here. If the command is
    // not available this is a no-op.
    if (ac::NetworkUtils::SendDriverPrivateCommand(mgmt_interface_->Ifname(),
                                                    BuildMiracastModeCommand(MiracastMode::kSource)) < 0)
        AC_WARNING("Failed to activate miracast mode of WiFi driver (not supported?)");
    else
        AC_DEBUG("Enabled WiFi driver miracast mode");

    auto sp = shared_from_this();

    if (current_device_->Role() == "GO")
        dhcp_server_ = w11tng::DhcpServer::Create(sp, ifname);
    else
        dhcp_client_ = w11tng::DhcpClient::Create(sp, ifname);
}

void NetworkManager::OnHostnameChanged() {
    AC_DEBUG("");
    SyncDeviceConfiguration();
}

void NetworkManager::OnRfkillChanged(const RfkillManager::Type &type) {
    if (type != RfkillManager::Type::kWLAN)
        return;

    if (delegate_)
        delegate_->OnReadyChanged();
}

} // namespace w11tng
