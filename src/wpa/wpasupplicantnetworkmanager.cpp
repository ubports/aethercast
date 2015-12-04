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

#include <sys/select.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <sys/prctl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <algorithm>
#include <chrono>

#include <boost/filesystem.hpp>

#include "wpasupplicantnetworkmanager.h"

#include "mcs/networkdevice.h"
#include "mcs/networkutils.h"
#include "mcs/utils.h"
#include "mcs/wfddeviceinfo.h"

namespace {
constexpr const char *kWpaSupplicantBinPath{"/sbin/wpa_supplicant"};
constexpr const uint kReadBufferSize{1024};
const std::chrono::milliseconds kDhcpIpAssignmentTimeout{5000};
const std::chrono::milliseconds kPeerFailureTimeout{5000};
const uint kSupplicantRespawnLimit{10};
const std::chrono::milliseconds kSupplicantRespawnTimeout{2000};

constexpr const char *kP2pDeviceFound{"P2P-DEVICE-FOUND"};
constexpr const char *kP2pDeviceLost{"P2P-DEVICE-LOST"};
constexpr const char *kP2pGroupFormationSuccess{"P2P-GROUP-FORMATION-SUCCESS"};
constexpr const char *kP2pGroupStarted{"P2P-GROUP-STARTED"};
constexpr const char *kP2pGroupRemoved{"P2P-GROUP-REMOVED"};
constexpr const char *kP2pGoNegFailure{"P2P-GO-NEG-FAILURE"};
}

WpaSupplicantNetworkManager::WpaSupplicantNetworkManager(NetworkManager::Delegate *delegate) :
    delegate_(delegate),
    // This network manager implementation is bound to the p2p0 network interface
    // being available which is the case on most Android platforms.
    interface_name_("p2p0"),
    firmware_loader_(interface_name_, this),
    ctrl_path_(mcs::Utils::Sprintf("/var/run/%s_supplicant", interface_name_.c_str())),
    command_queue_(new WpaSupplicantCommandQueue(this)),
    dhcp_client_(this, interface_name_),
    dhcp_server_(nullptr, interface_name_),
    channel_(nullptr)   ,
    channel_watch_(0),
    dhcp_timeout_(0),
    respawn_limit_(kSupplicantRespawnLimit),
    respawn_source_(0),
    is_group_owner_(false),
    scanning_(false) {
}

WpaSupplicantNetworkManager::~WpaSupplicantNetworkManager() {
    DisconnectSupplicant();
    StopSupplicant();

    if (respawn_source_)
        g_source_remove(respawn_source_);
}


bool WpaSupplicantNetworkManager::Setup() {
    if (!firmware_loader_.IsNeeded())
        return StartSupplicant();

    return firmware_loader_.TryLoad();
}

void WpaSupplicantNetworkManager::OnFirmwareLoaded() {
    StartSupplicant();
}

void WpaSupplicantNetworkManager::OnFirmwareUnloaded() {
    StopSupplicant();

    // FIXME what are we going to do now? This needs to be
    // solved together with the other system components
    // changing the firmware. Trying to reload the firmware
    // is the best we can do for now.
    firmware_loader_.TryLoad();
}

void WpaSupplicantNetworkManager::OnUnsolicitedResponse(WpaSupplicantMessage message) {
    if (message.Type() != kEvent) {
        g_warning("unhandled supplicant message: %s", message.Raw().c_str());
        return;
    }

    if (message.Name() == kP2pDeviceFound)
        OnP2pDeviceFound(message);
    else if (message.Name() == kP2pDeviceLost)
        OnP2pDeviceLost(message);
    else if (message.Name() == kP2pGroupStarted)
        OnP2pGroupStarted(message);
    else if (message.Name() == kP2pGroupRemoved)
        OnP2pGroupRemoved(message);
    else if (message.Name() == kP2pGoNegFailure)
        OnP2pGoNegFailure(message);
    else
        g_warning("unhandled supplicant event: %s", message.Raw().c_str());
}

void WpaSupplicantNetworkManager::OnP2pDeviceFound(WpaSupplicantMessage &message) {
    // P2P-DEVICE-FOUND 4e:74:03:70:e2:c1 p2p_dev_addr=4e:74:03:70:e2:c1
    // pri_dev_type=8-0050F204-2 name='Aquaris M10' config_methods=0x188 dev_capab=0x5
    // group_capab=0x0 wfd_dev_info=0x00111c440032 new=1

    char *address = nullptr;
    char *name = nullptr;
    char *config_methods_str = nullptr;

    message.ReadDictEntry("p2p_dev_addr", 's', &address);
    message.ReadDictEntry("name", 's', &name);
    message.ReadDictEntry("config_methods", 's', &config_methods_str);

    g_warning("Found device with address %s name %s config_methods %s", address, name, config_methods_str);

    // Check if we've that peer already in our list, if that is the
    // case we just update it.
    for (auto iter : available_devices_) {
        auto peer = iter.second;

        if (peer->Address() != address)
            continue;

        peer->SetAddress(address);
        peer->SetName(name);

        return;
    }

    mcs::NetworkDevice::Ptr peer(new mcs::NetworkDevice);
    peer->SetAddress(address);
    peer->SetName(name);

    available_devices_[address] = peer;

    if (delegate_)
        delegate_->OnDeviceFound(peer);
}

void WpaSupplicantNetworkManager::OnP2pDeviceLost(WpaSupplicantMessage &message) {
    // P2P-DEVICE-LOST p2p_dev_addr=4e:74:03:70:e2:c1

    const char *address = nullptr;

    message.Read("e", &address);

    auto peer = available_devices_[address];
    if (!peer)
        return;

    if (delegate_)
        delegate_->OnDeviceLost(peer);
}

void WpaSupplicantNetworkManager::OnP2pGroupStarted(WpaSupplicantMessage &message) {
    // P2P-GROUP-STARTED p2p0 GO ssid="DIRECT-hB" freq=2412 passphrase="HtP0qYon"
    // go_dev_addr=4e:74:03:64:95:a7
    if (!current_peer_.get())
        return;

    const char *role = nullptr;

    message.Skip("s");
    message.Read("s", &role);

    AdvanceDeviceState(current_peer_, mcs::kConfiguration);

    // If we're the GO the other side is the client and vice versa
    if (g_strcmp0(role, "GO") == 0) {
        is_group_owner_ = true;

        // As we're the owner we can now just startup the DHCP server
        // and report we're connected as there is not much more to do
        // from our side.
        dhcp_server_.Start();

        AdvanceDeviceState(current_peer_, mcs::kConnected);
    } else {
        is_group_owner_ = false;

        // We're a client of a formed group now and have to acquire
        // our IP address via DHCP so we have to wait until we're
        // reporting our upper layers that we're connected.
        dhcp_client_.Start();

        // To not wait forever we're starting a timeout here which
        // will bring everything down if we didn't received a IP
        // address once it happens.
        dhcp_timeout_ = g_timeout_add(kDhcpIpAssignmentTimeout.count(), &WpaSupplicantNetworkManager::OnGroupClientDhcpTimeout, this);
    }
}

void WpaSupplicantNetworkManager::OnP2pGroupRemoved(WpaSupplicantMessage &message) {
    // P2P-GROUP-REMOVED p2p0 GO reason=FORMATION_FAILED
    if (current_peer_.get())
        return;

    const char *reason = nullptr;

    message.ReadDictEntry("reason", 's', &reason);

    current_peer_->SetAddress(mcs::MacAddress(""));
    if (g_strcmp0(reason, "FORMATION_FAILED") == 0 ||
            g_strcmp0(reason, "PSK_FAILURE") == 0 ||
            g_strcmp0(reason, "FREQ_CONFLICT") == 0)
        AdvanceDeviceState(current_peer_, mcs::kFailure);
    else
        AdvanceDeviceState(current_peer_, mcs::kDisconnected);

    current_peer_.reset();
}

void WpaSupplicantNetworkManager::OnP2pGoNegFailure(WpaSupplicantMessage &message) {
    if (!current_peer_.get())
        return;

    AdvanceDeviceState(current_peer_, mcs::kFailure);

    current_peer_.reset();
}

void WpaSupplicantNetworkManager::OnWriteMessage(WpaSupplicantMessage message) {
    auto data = message.Raw();
    if (send(sock_, data.c_str(), data.length(), 0) < 0)
        g_warning("Failed to send data to wpa-supplicant");
}

mcs::IpV4Address WpaSupplicantNetworkManager::LocalAddress() const {
    mcs::IpV4Address address;

    if (is_group_owner_)
        address = dhcp_server_.LocalAddress();
    else
        address = dhcp_client_.LocalAddress();

    return address;
}

bool WpaSupplicantNetworkManager::Running() const {
    return supplicant_pid_ > 0;
}

bool WpaSupplicantNetworkManager::Scanning() const {
    return scanning_;
}

gboolean WpaSupplicantNetworkManager::OnConnectSupplicant(gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);
    // If we're no able to connect to supplicant we try it again next time
    return !inst->ConnectSupplicant();
}

void WpaSupplicantNetworkManager::OnSupplicantWatch(GPid pid, gint status, gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);

    g_warning("Supplicant process exited with status %d", status);

    if (!g_spawn_check_exit_status(status, nullptr))
        inst->HandleSupplicantFailed();
}

gboolean WpaSupplicantNetworkManager::OnSupplicantRespawn(gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);

    if (!inst->StartSupplicant() && inst->respawn_limit_ > 0) {
        // If we directly failed to start supplicant we schedule the next try
        // right away
        inst->respawn_limit_--;
        return TRUE;
    }

    return FALSE;
}

void WpaSupplicantNetworkManager::HandleSupplicantFailed() {
    if (respawn_limit_ > 0) {
        if (respawn_source_)
            g_source_remove(respawn_source_);

        respawn_source_ = g_timeout_add(kSupplicantRespawnTimeout.count(), &WpaSupplicantNetworkManager::OnSupplicantRespawn, this);
        respawn_limit_--;
    }

    DisconnectSupplicant();
    StopSupplicant();
    Reset();
}

void WpaSupplicantNetworkManager::Reset() {
    if (current_peer_) {
        AdvanceDeviceState(current_peer_, mcs::kDisconnected);

        current_peer_ = nullptr;

        if (dhcp_timeout_ > 0) {
            g_source_remove(dhcp_timeout_);
            dhcp_timeout_ = 0;
        }

        dhcp_client_.Stop();
        dhcp_server_.Stop();
    }

    if (delegate_) {
        for (auto peer : available_devices_)
            delegate_->OnDeviceLost(peer.second);
    }

    available_devices_.clear();
    is_group_owner_ = false;
}

bool WpaSupplicantNetworkManager::CreateSupplicantConfig(const std::string &conf_path) {
    auto config = mcs::Utils::Sprintf(
                "# GENERATED - DO NOT EDIT!\n"
                "config_methods=pbc\n" // We're only supporting PBC for now
                "ap_scan=1\n"
                "device_name=%s",
                "unknown");

    GError *error = nullptr;
    if (!g_file_set_contents(conf_path.c_str(), config.c_str(), config.length(), &error)) {
        g_warning("Failed to create configuration file for supplicant: %s",
                  error->message);
        g_error_free(error);
        return false;
    }

    return true;
}

void WpaSupplicantNetworkManager::OnSupplicantProcessSetup(gpointer user_data) {
    // Die when our parent dies so we don't stay around any longer and can
    // be restarted when the service restarts
    if (prctl(PR_SET_PDEATHSIG, SIGHUP) < 0)
        g_warning("Failed to track parents process status: %s", strerror(errno));
}

bool WpaSupplicantNetworkManager::StartSupplicant() {
    auto conf_path = mcs::Utils::Sprintf("/tmp/supplicant-%s.conf", interface_name_.c_str());

    if (!CreateSupplicantConfig(conf_path))
        return false;

    // Drop any left over control socket to be able to setup
    // a new one.
    boost::system::error_code err_code;
    auto path = boost::filesystem::path(ctrl_path_);
    boost::filesystem::remove_all(path, err_code);
    if (err_code)
        g_warning("Failed remove control directory for supplicant. Will cause problems.");

    auto cmdline = mcs::Utils::Sprintf("%s -Dnl80211 -i%s -C%s -ddd -t -K -c%s",
                                           kWpaSupplicantBinPath,
                                           interface_name_.c_str(),
                                           ctrl_path_.c_str(),
                                           conf_path.c_str());
    auto argv = g_strsplit(cmdline.c_str(), " ", -1);

    auto flags = G_SPAWN_DEFAULT | G_SPAWN_DO_NOT_REAP_CHILD;

    if (!getenv("MIRACAST_SUPPLICANT_DEBUG"))
        flags |= (G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL);

    GError *error = nullptr;
    int err = g_spawn_async(NULL, argv, NULL, (GSpawnFlags) flags,
                            &WpaSupplicantNetworkManager::OnSupplicantProcessSetup, NULL, &supplicant_pid_, &error);
    if (err < 0) {
        g_warning("Failed to spawn wpa-supplicant process: %s", error->message);
        g_strfreev(argv);
        g_error_free(error);
        return false;
    }

    err = g_child_watch_add(supplicant_pid_, &WpaSupplicantNetworkManager::OnSupplicantWatch, this);
    if (err < 0) {
        g_warning("Failed to setup watch for supplicant");
        StopSupplicant();
        return false;
    }

    g_strfreev(argv);

    g_timeout_add(500, &WpaSupplicantNetworkManager::OnConnectSupplicant, this);

    return true;
}

void WpaSupplicantNetworkManager::StopSupplicant() {
    if (supplicant_pid_ < 0)
        return;

    g_spawn_close_pid(supplicant_pid_);
    supplicant_pid_ = 0;
}

bool WpaSupplicantNetworkManager::ConnectSupplicant() {
    std::string socket_path = mcs::Utils::Sprintf("%s/%s",
                                                      ctrl_path_.c_str(),
                                                      interface_name_.c_str());

    g_warning("Connecting supplicant on %s", socket_path.c_str());

    struct sockaddr_un local;
    sock_ = ::socket(PF_UNIX, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        g_warning("Failed to create socket");
        return false;
    }

    local.sun_family = AF_UNIX;

    std::string local_path = mcs::Utils::Sprintf("/tmp/p2p0-%d", getpid());
    if (g_file_test(local_path.c_str(), G_FILE_TEST_EXISTS))
        g_remove(local_path.c_str());

    strncpy(local.sun_path, local_path.c_str(), sizeof(local.sun_path));

    if (::bind(sock_, (struct sockaddr *) &local, sizeof(local)) < 0) {
        g_warning("Failed to bind socket");
        return false;
    }

    struct sockaddr_un dest;
    dest.sun_family = AF_UNIX;
    strncpy(dest.sun_path, socket_path.c_str(), sizeof(dest.sun_path));

    if (::connect(sock_, (struct sockaddr*) &dest, sizeof(dest)) < 0) {
        g_warning("Failed to connect socket");
        return false;
    }

    int flags = ::fcntl(sock_, F_GETFL);
    flags |= O_NONBLOCK;
    ::fcntl(sock_, F_SETFL, flags);

    channel_ = g_io_channel_unix_new(sock_);
    channel_watch_ = g_io_add_watch(channel_, (GIOCondition) (G_IO_IN | G_IO_HUP | G_IO_ERR),
                       &WpaSupplicantNetworkManager::OnIncomingMessages, this);
    if (channel_watch_ == 0) {
        g_warning("Failed to setup watch for incoming messages from wpa-supplicant");
        return false;
    }

    // We need to attach to receive all occuring events from wpa-supplicant
    auto m = WpaSupplicantMessage::CreateRequest("ATTACH");
    RequestAsync(m, [=](const WpaSupplicantMessage &message) {
        if (message.IsFail()) {
            g_warning("Failed to attach to wpa-supplicant for unsolicited events");
            return;
        }
    });

    // Enable WiFi display support
    m = WpaSupplicantMessage::CreateRequest("SET");
    g_assert(m.Append("si", "wifi_display", 1));
    RequestAsync(m, [=](const WpaSupplicantMessage &message) { });

    std::list<std::string> wfd_sub_elements;
    // FIXME build this rather than specifying a static string here
    wfd_sub_elements.push_back(std::string("000600101C440032"));
    SetWfdSubElements(wfd_sub_elements);

    respawn_limit_ = kSupplicantRespawnLimit;

    return true;
}

void WpaSupplicantNetworkManager::DisconnectSupplicant() {
    if (sock_ < 0)
        return;

    if (channel_) {
        g_io_channel_shutdown(channel_, FALSE, nullptr);
        g_io_channel_unref(channel_);
        channel_ = nullptr;
    }

    if (channel_watch_ > 0) {
        g_source_remove(channel_watch_);
        channel_watch_ = 0;
    }

    if (sock_ > 0) {
        ::close(sock_);
        sock_ = 0;
    }
}

gboolean WpaSupplicantNetworkManager::OnIncomingMessages(GIOChannel *source, GIOCondition condition,
                                                       gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);
    char buf[kReadBufferSize];

    if (condition & G_IO_HUP) {
        g_warning("Got disconnected from supplicant");
        inst->StopSupplicant();
        return TRUE;
    }

    while (mcs::NetworkUtils::BytesAvailableToRead(inst->sock_) > 0) {
        int ret = recv(inst->sock_, buf, sizeof(buf) - 1, 0);
        if (ret < 0)
            return TRUE;

        buf[ret] = '\0';

        inst->command_queue_->HandleMessage(WpaSupplicantMessage::CreateRaw(buf));
    }

    return TRUE;
}

void WpaSupplicantNetworkManager::RequestAsync(const WpaSupplicantMessage &message, std::function<void(WpaSupplicantMessage)> callback) {
    command_queue_->EnqueueCommand(message, callback);
}

void WpaSupplicantNetworkManager::OnAddressAssigned(const mcs::IpV4Address &address) {
    if (!current_peer_)
        return;

    if (dhcp_timeout_ > 0) {
        g_source_remove(dhcp_timeout_);
        dhcp_timeout_ = 0;
    }


    AdvanceDeviceState(current_peer_, mcs::kConnected);
}

gboolean WpaSupplicantNetworkManager::OnDeviceFailureTimeout(gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);
    inst->current_peer_->SetState(mcs::kIdle);

    return FALSE;
}

gboolean WpaSupplicantNetworkManager::OnGroupClientDhcpTimeout(gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);

    if (!inst->current_peer_)
        return FALSE;

    // Switch peer back into idle state after some time
    g_timeout_add(kPeerFailureTimeout.count(), &WpaSupplicantNetworkManager::OnDeviceFailureTimeout, inst);

    inst->AdvanceDeviceState(inst->current_peer_, mcs::kFailure);

    return FALSE;
}

void WpaSupplicantNetworkManager::SetWfdSubElements(const std::list<std::string> &elements) {
    int n = 0;
    for (auto element : elements) {
        auto m = WpaSupplicantMessage::CreateRequest("WFD_SUBELEM_SET");
        m.Append("is", n, element.c_str());
        RequestAsync(m, [=](const WpaSupplicantMessage &message) { });
        n++;
    }
}

void WpaSupplicantNetworkManager::Scan() {
    auto m = WpaSupplicantMessage::CreateRequest("P2P_FIND");
    RequestAsync(m, [=](const WpaSupplicantMessage &message) {
        scanning_ = !message.IsFail();
    });
}

void WpaSupplicantNetworkManager::StopScan() {
    auto m = WpaSupplicantMessage::CreateRequest("P2P_STOP_FIND");
    RequestAsync(m, [=](const WpaSupplicantMessage &message) {
        scanning_ = message.IsFail();
    });
}

std::vector<mcs::NetworkDevice::Ptr> WpaSupplicantNetworkManager::Devices() const {
    std::vector<mcs::NetworkDevice::Ptr> values;
    std::transform(available_devices_.begin(), available_devices_.end(),
                   std::back_inserter(values),
                   [=](const std::pair<std::string,mcs::NetworkDevice::Ptr> &value) {
        return value.second;
    });
    return values;
}

void WpaSupplicantNetworkManager::AdvanceDeviceState(const mcs::NetworkDevice::Ptr &device, mcs::NetworkDeviceState state) {
    device->SetState(state);
    if (delegate_)
        delegate_->OnDeviceStateChanged(device);
}

bool WpaSupplicantNetworkManager::Connect(const mcs::NetworkDevice::Ptr &device) {
    if (available_devices_.find(device->Address()) == available_devices_.end())
        return false;

    if (current_peer_.get())
        return false;

    current_peer_ = available_devices_[device->Address()];

    auto m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");
    m.Append("ss", device->Address().c_str(), "pbc");

    RequestAsync(m, [&](const WpaSupplicantMessage &message) {
        if (message.IsFail()) {
            AdvanceDeviceState(current_peer_, mcs::kFailure);
            return;
        }
    });

    return true;
}

bool WpaSupplicantNetworkManager::DisconnectAll() {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRequest("P2P_GROUP_REMOVE");
    m.Append("s", interface_name_.c_str());

    RequestAsync(m, [&](const WpaSupplicantMessage &message) {
        if (message.IsFail()) {
            g_warning("Failed to disconnect all connected devices on interface %s", interface_name_.c_str());
            return;
        }
    });

    return true;
}
