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

#ifndef W11TNG_NETWORKMANAGER_H_
#define W11TNG_NETWORKMANAGER_H_

#include <unordered_map>

#include <ac/networkmanager.h>

#include "managerstub.h"
#include "p2pdevicestub.h"
#include "interfacestub.h"
#include "interfaceselector.h"
#include "dhcpserver.h"
#include "dhcpclient.h"
#include "wififirmwareloader.h"
#include "informationelement.h"
#include "hostname1stub.h"
#include "rfkillmanager.h"

namespace w11tng {

class NetworkManager : public std::enable_shared_from_this<NetworkManager>,
                       public ac::NetworkManager,
                       public P2PDeviceStub::Delegate,
                       public NetworkDevice::Delegate,
                       public w11tng::DhcpClient::Delegate,
                       public w11tng::DhcpServer::Delegate,
                       public w11tng::WiFiFirmwareLoader::Delegate,
                       public w11tng::InterfaceSelector::Delegate,
                       public w11tng::ManagerStub::Delegate,
                       public w11tng::InterfaceStub::Delegate,
                       public w11tng::Hostname1Stub::Delegate,
                       public w11tng::RfkillManager::Delegate {
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};

    static ac::NetworkManager::Ptr Create();

    ~NetworkManager();

    void SetDelegate(ac::NetworkManager::Delegate * delegate) override;

    bool Setup() override;
    void Release() override;

    void Scan(const std::chrono::seconds &timeout) override;
    bool Connect(const ac::NetworkDevice::Ptr &device) override;
    bool Disconnect(const ac::NetworkDevice::Ptr &device) override;

    std::vector<ac::NetworkDevice::Ptr> Devices() const override;
    ac::IpV4Address LocalAddress() const override;
    bool Running() const override;
    bool Scanning() const override;
    bool Ready() const override;

    void SetCapabilities(const std::vector<Capability> &capabilities);
    std::vector<Capability> Capabilities() const;
    void OnP2PDeviceChanged() override;
    void OnP2PDeviceReady() override;

    void OnDeviceFound(const std::string &path) override;
    void OnDeviceLost(const std::string &path) override;
    void OnPeerConnectFailed() override;
    void OnGroupOwnerNegotiationFailure(const std::string &peer_path, const P2PDeviceStub::GroupOwnerNegotiationResult &result) override;
    void OnGroupOwnerNegotiationSuccess(const std::string &peer_path, const P2PDeviceStub::GroupOwnerNegotiationResult &result) override;
    void OnGroupStarted(const std::string &group_path, const std::string &interface_path, const std::string &role) override;
    void OnGroupFinished(const std::string &group_path, const std::string &interface_path) override;
    void OnGroupRequest(const std::string &peer_path, int dev_passwd_id) override;

    void OnDeviceChanged(const NetworkDevice::Ptr &device) override;
    void OnDeviceReady(const NetworkDevice::Ptr &device) override;

    void OnDhcpAddressAssigned(const ac::IpV4Address &local_address, const ac::IpV4Address &remote_address) override;
    void OnDhcpTerminated();

    void OnFirmwareLoaded() override;
    void OnFirmwareUnloaded() override;

    void OnInterfaceSelectionDone(const std::string &path) override;

    void OnManagerReady() override;
    void OnManagerInterfaceAdded(const std::string &path) override;
    void OnManagerInterfaceRemoved(const std::string &path) override;
    void OnManagerInterfaceCreationFailed() override;

    void OnInterfaceReady(const std::string &object_path) override;

    void OnHostnameChanged() override;

    void OnRfkillChanged(const RfkillManager::Type &type) override;

private:
    static void OnServiceLost(GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void OnServiceFound(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data);

    static void OnURfkillAvailable(GDBusConnection*, const gchar*, const gchar*, gpointer user_data);
    static void OnURfkillNotAvailable(GDBusConnection*, const gchar*, gpointer user_data);

private:
    NetworkManager();
    std::shared_ptr<NetworkManager> FinalizeConstruction();

    void FinishRfkillInitialization();

    NetworkDevice::Ptr FindDevice(const std::string &address);
    void Initialize(bool firmware_loading = false);
    void ReleaseInternal();
    void ReleaseInterface();
    void SetupInterface(const std::string &object_path);
    void AdvanceDeviceState(const NetworkDevice::Ptr &device, ac::NetworkDeviceState state);
    void ConfigureFromCapabilities();

    void StartConnectTimeout();
    void StopConnectTimeout();

    DeviceType GenerateWfdDeviceType();

    std::string SelectHostname();
    std::string SelectDeviceType();
    void SyncDeviceConfiguration();

    void HandleConnectFailed();

    void OnGroupInterfaceReady();
    void OnManagementInterfaceReady();

    enum class MiracastMode : int {
        kOff = 0,
        kSource = 1,
        kSink = 2
    };

    std::string BuildMiracastModeCommand(MiracastMode mode);

private:
    ac::ScopedGObject<GDBusConnection> connection_;
    ac::NetworkManager::Delegate *delegate_;
    std::shared_ptr<ManagerStub> manager_;
    std::shared_ptr<InterfaceStub> mgmt_interface_;
    std::shared_ptr<P2PDeviceStub> p2p_device_;
    std::unordered_map<std::string,w11tng::NetworkDevice::Ptr> devices_;
    NetworkDevice::Ptr current_device_;
    InterfaceStub::Ptr current_group_iface_;
    P2PDeviceStub::Ptr current_group_device_;
    std::shared_ptr<w11tng::DhcpClient> dhcp_client_;
    std::shared_ptr<w11tng::DhcpServer> dhcp_server_;
    InterfaceSelector::Ptr interface_selector_;
    guint connect_timeout_;
    w11tng::WiFiFirmwareLoader firmware_loader_;
    std::string dedicated_p2p_interface_;
    bool session_available_;
    std::vector<Capability> capabilities_;
    Hostname1Stub::Ptr hostname_service_;
    RfkillManager::Ptr rfkill_manager_;
    guint urfkill_watch_;
};

} // namespace w11tng

#endif
