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

#include <mcs/logger.h>
#include <mcs/keep_alive.h>

#include "networkmanager.h"
#include "informationelement.h"

namespace w11tng {

mcs::NetworkManager::Ptr NetworkManager::Create() {
    return std::shared_ptr<NetworkManager>(new NetworkManager())->FinalizeConstruction();
}

std::shared_ptr<NetworkManager> NetworkManager::FinalizeConstruction() {
    auto sp = shared_from_this();

    GError *error = nullptr;
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!connection_) {
        MCS_ERROR("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    return sp;
}

NetworkManager::NetworkManager() :
    firmware_loader_("", this),
    dedicated_p2p_interface_(mcs::Utils::GetEnvValue("AETHERCAST_DEDICATED_P2P_INTERFACE")) {
}

NetworkManager::~NetworkManager() {
}

void NetworkManager::OnServiceFound(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data) {
    boost::ignore_unused_variable_warning(connection);
    boost::ignore_unused_variable_warning(name);
    boost::ignore_unused_variable_warning(name_owner);

    auto inst = static_cast<mcs::WeakKeepAlive<NetworkManager>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    inst->Initialize(true);
}

void NetworkManager::OnServiceLost(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    boost::ignore_unused_variable_warning(connection);
    boost::ignore_unused_variable_warning(name);

    auto inst = static_cast<mcs::WeakKeepAlive<NetworkManager>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    inst->Release();
}

void NetworkManager::Initialize(bool firmware_loading) {
    MCS_DEBUG("");

    if (firmware_loading && mcs::Utils::GetEnvValue("AETHERCAST_NEED_FIRMWARE") == "1") {
        auto interface_name = mcs::Utils::GetEnvValue("AETHERCAST_DEDICATED_P2P_INTERFACE");
        if (interface_name.length() == 0)
            interface_name = "p2p0";

        firmware_loader_.SetInterfaceName(interface_name);
        if (firmware_loader_.IsNeeded()) {
            MCS_DEBUG("Loading WiFi firmware for interface %s", interface_name);
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
}

void NetworkManager::SetupInterface(const std::string &object_path) {
    if (p2p_device_)
        return;

    p2p_device_ = P2PDeviceStub::Create(object_path, shared_from_this());
}

void NetworkManager::Release() {
    MCS_DEBUG("");
}

void NetworkManager::SetDelegate(mcs::NetworkManager::Delegate *delegate) {
    delegate_ = delegate;
}

bool NetworkManager::Setup() {
    g_bus_watch_name_on_connection(connection_.get(),
                                   kBusName,
                                   G_BUS_NAME_WATCHER_FLAGS_NONE,
                                   &NetworkManager::OnServiceFound,
                                   &NetworkManager::OnServiceLost,
                                   new mcs::WeakKeepAlive<NetworkManager>(shared_from_this()),
                                   nullptr);

    return false;
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
    MCS_DEBUG("");

    connect_timeout_ = g_timeout_add_seconds(kConnectTimeout, [](gpointer user_data) {
        auto inst = static_cast<mcs::SharedKeepAlive<NetworkManager>*>(user_data)->ShouldDie();
        if (not inst)
            return FALSE;

        MCS_WARNING("Connect timeout happened");

        inst->connect_timeout_ = 0;

        // If the device is either already connected or trying to get an address
        // over DHCP we don't do anything. DHCP process will fail on its own after
        // some time and we will react on this.
        if (inst->current_device_->State() == mcs::kConnected ||
            inst->current_device_->State() == mcs::kConfiguration)
            return FALSE;

        inst->p2p_device_->Cancel();

        inst->AdvanceDeviceState(inst->current_device_, mcs::kFailure);
        inst->current_device_.reset();

        // We don't have an active group if we're not in connected or configuration
        // state so we don't have to care about terminating any group at this point.

        return FALSE;
    }, new mcs::SharedKeepAlive<NetworkManager>{shared_from_this()});
}

void NetworkManager::StopConnectTimeout() {
    if (connect_timeout_ == 0)
        return;

    MCS_DEBUG("");

    g_source_remove(connect_timeout_);
    connect_timeout_ = 0;
}

bool NetworkManager::Connect(const mcs::NetworkDevice::Ptr &device) {
    if (!p2p_device_ || current_device_)
        return false;

    MCS_DEBUG("address %s", device->Address());

    // Lets check here if we really own this device and if yes then we
    // select our own instance of it rather than relying on the input
    auto d = FindDevice(device->Address());
    if (!d) {
        MCS_WARNING("Could not find instance for device %s", device->Address());
        return false;
    }

    current_device_ = d;

    p2p_device_->StopFind();

    if (!p2p_device_->Connect(d->ObjectPath()))
        return false;

    current_device_->SetState(mcs::kAssociation);
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

    return mcs::Utils::Sprintf("%s%s%s", category, oui, sub_category);
}

void NetworkManager::SyncDeviceConfiguration() {
    if (!p2p_device_)
        return;

    auto hostname = SelectHostname();
    auto device_type = SelectDeviceType();

    p2p_device_->SetDeviceConfiguration(hostname, device_type);
}

bool NetworkManager::Disconnect(const mcs::NetworkDevice::Ptr &device) {
    if (!p2p_device_ || !current_device_)
        return false;

    if (!FindDevice(device->Address()))
        return false;

    // This will trigger the GroupFinished signal where we will release
    // all parts in order.
    current_group_device_->Disconnect();

    return true;
}

void NetworkManager::SetWfdSubElements(const std::list<std::string> &elements) {
    MCS_WARNING("Not implemeneted");
}

std::vector<mcs::NetworkDevice::Ptr> NetworkManager::Devices() const {
    std::vector<mcs::NetworkDevice::Ptr> values;
    std::transform(devices_.begin(), devices_.end(),
                   std::back_inserter(values),
                   [=](const std::pair<std::string,w11tng::NetworkDevice::Ptr> &value) {
        return value.second;
    });
    return values;
}

mcs::IpV4Address NetworkManager::LocalAddress() const {
    mcs::IpV4Address address;

    if (dhcp_server_)
        address = dhcp_server_->LocalAddress();
    else if (dhcp_client_)
        address = dhcp_client_->LocalAddress();

    MCS_DEBUG("address %s", address);

    return address;
}

bool NetworkManager::Running() const {
    return p2p_device_ && p2p_device_->Connected();
}

bool NetworkManager::Scanning() const {
    return p2p_device_ && p2p_device_->Scanning();
}

void NetworkManager::OnP2PDeviceChanged() {
    if (delegate_)
        delegate_->OnChanged();
}

void NetworkManager::OnP2PDeviceReady() {
    MCS_DEBUG("");
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

    MCS_DEBUG("peer %s", path);

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

void NetworkManager::AdvanceDeviceState(const NetworkDevice::Ptr &device, mcs::NetworkDeviceState state) {
    device->SetState(state);
    if (delegate_)
        delegate_->OnDeviceStateChanged(device);
}

void NetworkManager::OnGroupOwnerNegotiationFailure(const std::string &peer_path) {
    if (!current_device_)
        return;

    MCS_DEBUG("peer %s", peer_path);

    AdvanceDeviceState(current_device_, mcs::kFailure);

    current_device_.reset();

    StopConnectTimeout();
}

void NetworkManager::OnGroupOwnerNegotiationSuccess(const std::string &peer_path) {
    if (!current_device_)
        return;

    MCS_DEBUG("peer %s", peer_path);
}

void NetworkManager::OnGroupStarted(const std::string &group_path, const std::string &interface_path, const std::string &role) {
    if (!current_device_)
        return;

    MCS_DEBUG("group %s interface %s role %s", group_path, interface_path, role);

    AdvanceDeviceState(current_device_, mcs::kConfiguration);

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

    MCS_DEBUG("group %s interface %s", group_path, interface_path);

    StopConnectTimeout();

    dhcp_client_.reset();
    dhcp_server_.reset();

    current_group_iface_.reset();
    current_group_device_.reset();

    AdvanceDeviceState(current_device_, mcs::kDisconnected);
    current_device_.reset();
}

void NetworkManager::OnGroupRequest(const std::string &peer_path, int dev_passwd_id) {
    MCS_DEBUG("peer %s dev_passwd_id %d", peer_path, dev_passwd_id);

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

void NetworkManager::OnAddressAssigned(const mcs::IpV4Address &address) {
    if (!current_device_ || current_device_->State() != mcs::kConfiguration)
        return;

    MCS_DEBUG("address %s", address);

    current_device_->SetIpV4Address(address);

    StopConnectTimeout();

    AdvanceDeviceState(current_device_, mcs::kConnected);
}

void NetworkManager::OnNoLease() {
    if (!current_device_ || current_device_->State() != mcs::kConfiguration)
        return;

    MCS_DEBUG("");

    Disconnect(current_device_);
    AdvanceDeviceState(current_device_, mcs::kFailure);
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

    MCS_DEBUG("Found P2P interface %s", path);
    SetupInterface(path);
}

void NetworkManager::OnManagerReady() {
    InformationElement ie;
    auto sub_element = new_subelement(DEVICE_INFORMATION);
    auto dev_info = (DeviceInformationSubelement*) sub_element;

    dev_info->session_management_control_port = htons(7236);
    dev_info->maximum_throughput = htons(50);
    dev_info->field1.device_type = SOURCE;
    dev_info->field1.session_availability = true;
    ie.add_subelement(sub_element);

    auto ie_data = ie.serialize();

    manager_->SetWFDIEs(ie_data->bytes, ie_data->length);

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
    if (!current_group_iface_)
        return;

    if (path == current_group_iface_->ObjectPath()) {
        Disconnect(current_device_);
        AdvanceDeviceState(current_device_, mcs::kFailure);
    }
}

void NetworkManager::OnManagerInterfaceCreationFailed() {
    // When interface creation failed its most likely that we were
    // restarted and that the interface stayed available at wpa and
    // we can simply start and reuse it here.
    interface_selector_->Process(manager_->Interfaces());
}

void NetworkManager::OnInterfaceReady() {
    if (!current_device_ || current_device_->State() != mcs::kConfiguration)
        return;

    auto ifname = current_group_iface_->Ifname();

    MCS_DEBUG("interface %s", ifname);

    if (current_device_->Role() == "GO") {
        dhcp_server_ = w11tng::DhcpServer::Create(nullptr, ifname);
        dhcp_server_->Start();

        StopConnectTimeout();

        // We don't have anything more to do when we play the owner
        // role. The core will setup the necessary endpoints and
        // waits for incoming connections.
        AdvanceDeviceState(current_device_, mcs::kConnected);
    }
    else {
        dhcp_client_ = w11tng::DhcpClient::Create(shared_from_this(), ifname);
        dhcp_client_->Start();
    }
}

void NetworkManager::OnHostnameChanged() {
    MCS_DEBUG("");
    SyncDeviceConfiguration();
}

} // namespace w11tng
