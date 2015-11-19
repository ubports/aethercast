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

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>

#include <algorithm>

#include "wpasupplicantnetworkmanager.h"

#include "networkdevice.h"
#include "wfddeviceinfo.h"

#include "networkutils.h"
#include "utilities.h"

#define WPA_SUPPLICANT_BIN_PATH     "/sbin/wpa_supplicant"

#define READ_BUFFER_SIZE            1024

#define DHCP_IP_ASSIGNMENT_TIMEOUT  5000 /* 5 seconds */
#define PEER_FAILURE_TIMEOUT        5000 /* 5 seconds */

/*
 * Supplicant message names
 */

#define P2P_DEVICE_FOUND                         "P2P-DEVICE-FOUND"
#define P2P_DEVICE_LOST                          "P2P-DEVICE-LOST"
#define P2P_GROUP_FORMATION_SUCCESS              "P2P-GROUP-FORMATION-SUCCESS"
#define P2P_GROUP_STARTED                        "P2P-GROUP-STARTED"
#define P2P_GROUP_REMOVED                        "P2P-GROUP-REMOVED"

WpaSupplicantNetworkManager::WpaSupplicantNetworkManager(NetworkManager::Delegate *delegate, const std::string &interface_name) :
    delegate_(delegate),
    interface_name_(interface_name),
    ctrl_path_(utilities::StringFormat("/var/run/%s_supplicant", interface_name_.c_str())),
    command_queue_(new WpaSupplicantCommandQueue(this)),
    current_role_(kUndecided),
    dhcp_client_(this, interface_name),
    dhcp_server_(nullptr, interface_name),
    channel_(nullptr),
    dhcp_timeout_(0) {
}

WpaSupplicantNetworkManager::~WpaSupplicantNetworkManager() {
    // FIXME
}

void WpaSupplicantNetworkManager::OnUnsolicitedResponse(WpaSupplicantMessage message) {
    g_warning("EVENT: %s", message.Dump().c_str());
    g_warning("EVENT NAME: %s", message.Name().c_str());

    if (message.Name() == P2P_DEVICE_FOUND) {
        // P2P-DEVICE-FOUND 4e:74:03:70:e2:c1 p2p_dev_addr=4e:74:03:70:e2:c1
        // pri_dev_type=8-0050F204-2 name='Aquaris M10' config_methods=0x188 dev_capab=0x5
        // group_capab=0x0 wfd_dev_info=0x00111c440032 new=1

        char *address = nullptr;
        char *name = nullptr;
        char *config_methods_str = nullptr;

        message.Read("s", &address);
        message.Skip("ee");
        message.Read("ee", &name, &config_methods_str);

#if 0
        auto wfdDevInfoStr = items[8].section('=', 1).mid(2);
        WfdDeviceInfo wfdDevInfo(parseHex(wfdDevInfoStr.left(4).toStdString()),
                              parseHex(wfdDevInfoStr.mid(4, 4).toStdString()),
                              parseHex(wfdDevInfoStr.right(4).toStdString()));
        auto configMethods = parseHex(items[5].section('=', 1).toStdString());

        qDebug() << "P2P device found:";
        qDebug() << "  address" << address.c_str()
                 << "deviceType" << wfdDevInfo.device_type_as_string().c_str()
                 << "controlPort" << wfdDevInfo.control_port()
                 << "maxThroughput" << wfdDevInfo.max_throughput()
                 << "configMethods" << configMethods;
#endif

        g_warning("Found device with address %s name %s config_methods %s", address, name, config_methods_str);

        // Check if we've that peer already in our list, if that is the
        // case we just update it.
        for (auto iter : available_devices_) {
            if (iter.second->Address() != std::string(address))
                continue;

            if (delegate_)
                delegate_->OnDeviceChanged(iter.second);

            return;
        }

        NetworkDevice::Ptr peer(new NetworkDevice);
        peer->SetAddress(address);
        peer->SetName(name);
        // peer->set_wfd_device_info(wfdDevInfo);
        peer->SetConfigMethods(utilities::ParseHex(std::string(config_methods_str)));

        available_devices_.insert(std::pair<std::string,NetworkDevice::Ptr>(std::string(address), NetworkDevice::Ptr(peer)));

        if (delegate_)
            delegate_->OnDeviceFound(peer);

        g_warning("Found new device");
    } else if (message.Name() == P2P_DEVICE_LOST) {
        // P2P-DEVICE-LOST p2p_dev_addr=4e:74:03:70:e2:c1

        const char *address = nullptr;

        message.Read("e", &address);

        auto peer = available_devices_[std::string(address)];
        if (!peer)
            return;

        if (delegate_)
            delegate_->OnDeviceLost(peer);

    } else if (message.Name() == P2P_GROUP_STARTED) {
        // P2P-GROUP-STARTED p2p0 GO ssid="DIRECT-hB" freq=2412 passphrase="HtP0qYon"
        // go_dev_addr=4e:74:03:64:95:a7
        if (!current_peer_.get())
            return;

        const char *role = nullptr;

        message.Skip("s");
        message.Read("s", &role);


        g_warning("Group started: role %s", role);

        current_peer_->SetState(kConfiguration);

        // If we're the GO the other side is the client and vice versa
        if (g_strcmp0(role, "GO") == 0) {
            current_role_ = kGroupOwner;

            current_peer_->SetState(kConnected);

            // As we're the owner we can now just startup the DHCP server
            // and report we're connected as there is not much more to do
            // from our side.
            dhcp_server_.Start();

            if (delegate_)
                delegate_->OnDeviceFound(current_peer_);
        } else {
            current_role_ = kGroupClient;

            // We're a client of a formed group now and have to acquire
            // our IP address via DHCP so we have to wait until we're
            // reporting our upper layers that we're connected.
            dhcp_client_.Start();

            // To not wait forever we're starting a timeout here which
            // will bring everything down if we didn't received a IP
            // address once it happens.
            dhcp_timeout_ = g_timeout_add(DHCP_IP_ASSIGNMENT_TIMEOUT, &WpaSupplicantNetworkManager::OnGroupClientDhcpTimeout, this);
        }
    } else if (message.Name() == P2P_GROUP_REMOVED) {
        // P2P-GROUP-REMOVED p2p0 GO reason=FORMATION_FAILED
        if (current_peer_.get())
            return;

        current_peer_->SetAddress("");
        current_peer_->SetState(kDisconnected);

        const char *reason = nullptr;

        message.Skip("ss");
        message.Read("e", &reason);

        if (delegate_) {
            if (g_strcmp0(reason, "FORMATION_FAILED") == 0 ||
                    g_strcmp0(reason, "PSK_FAILURE") == 0 ||
                    g_strcmp0(reason, "FREQ_CONFLICT") == 0)
                delegate_->OnDeviceFailed(current_peer_);
            else
                delegate_->OnDeviceDisconnected(current_peer_);
        }

        current_peer_.reset();
    }
}

void WpaSupplicantNetworkManager::OnWriteMessage(WpaSupplicantMessage message) {
    auto data = message.Raw();
    g_warning("Sending %s", data.c_str());
    if (send(sock_, data.c_str(), data.length(), 0) < 0)
        g_warning("Failed to send data to wpa-supplicant");
}

NetworkDeviceRole WpaSupplicantNetworkManager::Role() const {
    return current_role_;
}

std::string WpaSupplicantNetworkManager::LocalAddress() const {
    std::string address;

    if (current_role_ == kGroupOwner)
        address = dhcp_server_.LocalAddress();
    else
        address = dhcp_client_.LocalAddress();

    return address;
}

void WpaSupplicantNetworkManager::Setup() {
    StartService();
}

gboolean WpaSupplicantNetworkManager::OnSupplicantConnected(gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);
    inst->ConnectSupplicant();

    return FALSE;
}

void WpaSupplicantNetworkManager::OnSupplicantWatch(GPid pid, gint status, gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);

    if (status < 0)
        g_warning("Supplicant process failed with status %d", status);

    // FIXME bring everything down
}

void WpaSupplicantNetworkManager::StartService() {
    char buf[1024];
    g_snprintf(buf, 1024, "%s -Dnl80211 -i%s -C%s -ddd -t -K",
               WPA_SUPPLICANT_BIN_PATH,
               interface_name_.c_str(),
               ctrl_path_.c_str());

    auto argv = g_strsplit(buf, " ", -1);

    GError *error;
    int err = g_spawn_async(NULL, argv, NULL, (GSpawnFlags) (G_SPAWN_DEFAULT | G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_STDOUT_TO_DEV_NULL),
                            NULL, NULL, &supplicant_pid_, &error);
    if (err < 0) {
        g_warning("Failed to spawn wpa-supplicant process: %s", error->message);
        g_strfreev(argv);
        g_error_free(error);
        return;
    }

    err = g_child_watch_add(supplicant_pid_, &WpaSupplicantNetworkManager::OnSupplicantWatch, this);
    if (err < 0)
        g_warning("Failed to setup watch for supplicant");

    g_strfreev(argv);

    g_timeout_add(500, &WpaSupplicantNetworkManager::OnSupplicantConnected, this);
}

bool WpaSupplicantNetworkManager::CheckResult(const std::string &result) {
    if (result.length() == 0)
        return true;

    return result.length() == 3 && result == "OK\n";
}

void WpaSupplicantNetworkManager::ConnectSupplicant() {
    std::string socket_path = utilities::StringFormat("%s/%s",
                                                      ctrl_path_.c_str(),
                                                      interface_name_.c_str());

    g_warning("Connecting supplicant on %s", socket_path.c_str());

    struct sockaddr_un local;
    sock_ = ::socket(PF_UNIX, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        g_warning("Failed to create socket");
        return;
    }

    local.sun_family = AF_UNIX;

    std::string local_path = utilities::StringFormat("/tmp/p2p0-%d", getpid());
    strncpy(local.sun_path, local_path.c_str(), sizeof(local.sun_path));

    if (::bind(sock_, (struct sockaddr *) &local, sizeof(local)) < 0) {
        g_warning("Failed to bind socket");
        return;
    }

    struct sockaddr_un dest;
    dest.sun_family = AF_UNIX;
    strncpy(dest.sun_path, socket_path.c_str(), sizeof(dest.sun_path));

    if (::connect(sock_, (struct sockaddr*) &dest, sizeof(dest)) < 0) {
        g_warning("Failed to connect socket");
        return;
    }

    int flags = ::fcntl(sock_, F_GETFL);
    flags |= O_NONBLOCK;
    ::fcntl(sock_, F_SETFL, flags);

    channel_ = g_io_channel_unix_new(sock_);
    if (g_io_add_watch(channel_, (GIOCondition) (G_IO_IN | G_IO_HUP | G_IO_ERR),
                       &WpaSupplicantNetworkManager::OnIncomingMessages, this) < 0) {
        g_warning("Failed to setup watch for incoming messages from wpa-supplicant");
        return;
    }

    // We need to attach to receive all occuring events from wpa-supplicant
    auto m = WpaSupplicantMessage::CreateRequest("ATTACH");
    Request(m, [=](const WpaSupplicantMessage &message) {
        if (message.IsFail()) {
            g_warning("Failed to attach to wpa-supplicant for unsolicited events");
            return;
        }
    });

    // Enable WiFi display support
    m = WpaSupplicantMessage::CreateRequest("SET");
    g_assert(m.Append("si", "wifi_display", 1));
    Request(m, [=](const WpaSupplicantMessage &message) { });

    std::list<std::string> wfd_sub_elements;
    // FIXME build this rather than specifying a static string here
    wfd_sub_elements.push_back(std::string("000600101C440032"));
    SetWfdSubElements(wfd_sub_elements);
}

gboolean WpaSupplicantNetworkManager::OnIncomingMessages(GIOChannel *source, GIOCondition condition,
                                                       gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);
    char buf[READ_BUFFER_SIZE];

    while (NetworkUtils::BytesAvailableToRead(inst->sock_) > 0) {
        int ret = recv(inst->sock_, buf, sizeof(buf) - 1, 0);
        if (ret < 0)
            return TRUE;

        buf[ret] = '\0';

        inst->command_queue_->HandleMessage(WpaSupplicantMessage::CreateRaw(buf));
    }

    return TRUE;
}

void WpaSupplicantNetworkManager::Request(const WpaSupplicantMessage &message, std::function<void(WpaSupplicantMessage)> callback) {
    command_queue_->EnqueueCommand(message, callback);
}

void WpaSupplicantNetworkManager::OnAddressAssigned(const std::string &address) {
    if (!current_peer_)
        return;

    if (dhcp_timeout_ > 0) {
        g_source_remove(dhcp_timeout_);
        dhcp_timeout_ = 0;
    }

    current_peer_->SetAddress(address);
    current_peer_->SetState(kConnected);

    if (delegate_)
        delegate_->OnDeviceConnected(current_peer_);
}

gboolean WpaSupplicantNetworkManager::OnDeviceFailureTimeout(gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);
    inst->current_peer_->SetState(kIdle);

    return FALSE;
}

gboolean WpaSupplicantNetworkManager::OnGroupClientDhcpTimeout(gpointer user_data) {
    auto inst = static_cast<WpaSupplicantNetworkManager*>(user_data);

    if (!inst->current_peer_)
        return FALSE;

    inst->current_peer_->SetState(kFailure);

    // Switch peer back into idle state after some time
    g_timeout_add(PEER_FAILURE_TIMEOUT, &WpaSupplicantNetworkManager::OnDeviceFailureTimeout, inst);

    if (inst->delegate_)
        inst->delegate_->OnDeviceFailed(inst->current_peer_);

    return FALSE;
}

void WpaSupplicantNetworkManager::SetWfdSubElements(const std::list<std::string> &elements) {
    int n = 0;
    for (auto element : elements) {
        auto m = WpaSupplicantMessage::CreateRequest("WFD_SUBELEM_SET");
        m.Append("is", n, element.c_str());
        Request(m, [=](const WpaSupplicantMessage &message) { });
        n++;
    }
}

void WpaSupplicantNetworkManager::Scan(unsigned int timeout) {
    auto m = WpaSupplicantMessage::CreateRequest("P2P_FIND");
    m.Append("i", timeout);
    Request(m, [=](const WpaSupplicantMessage &message) { });
}

std::vector<NetworkDevice::Ptr> WpaSupplicantNetworkManager::Devices() const {
    std::vector<NetworkDevice::Ptr> values;
    std::transform(available_devices_.begin(), available_devices_.end(),
                   std::back_inserter(values),
                   [=](const std::pair<std::string,NetworkDevice::Ptr> &value) {
        return value.second;
    });
    return values;
}

int WpaSupplicantNetworkManager::Connect(const std::string &address, bool persistent) {
    int ret = 0;

    if (available_devices_.find(address) == available_devices_.end())
        return -EINVAL;

    g_warning("test1");

    if (current_peer_.get())
        return -EALREADY;

    g_warning("test2");

    current_peer_ = available_devices_[address];

    auto m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");
    m.Append("sss", address.c_str(), "pbc", persistent ? "persistent" : "");

    Request(m, [&](const WpaSupplicantMessage &message) {
        if (message.IsFail()) {
            ret = -EIO;
            g_warning("Failed to connect with remote %s", address.c_str());
            return;
        }
    });

    return ret;
}

int WpaSupplicantNetworkManager::DisconnectAll() {
    int ret = 0;

    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRequest("P2P_GROUP_REMOVE");
    m.Append("s", interface_name_.c_str());

    Request(m, [&](const WpaSupplicantMessage &message) {
        if (message.IsFail()) {
            ret = -EIO;
            g_warning("Failed to disconnect all connected devices on interface %s", interface_name_.c_str());
            return;
        }
    });

    return ret;
}
