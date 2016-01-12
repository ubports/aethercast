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

#include <mcs/networkmanager.h>
#include <w11t/dhcpclient.h>
#include <w11t/dhcpserver.h>
#include <w11t/wififirmwareloader.h>

#include "managerstub.h"
#include "p2pdevicestub.h"
#include "interfacestub.h"
#include "interfaceselector.h"

namespace w11tng {

class NetworkManager : public std::enable_shared_from_this<NetworkManager>,
                       public mcs::NetworkManager,
                       public P2PDeviceStub::Delegate,
                       public NetworkDevice::Delegate,
                       public w11t::DhcpClient::Delegate,
                       public w11t::WiFiFirmwareLoader::Delegate,
                       public w11tng::InterfaceSelector::Delegate,
                       public w11tng::ManagerStub::Delegate,
                       public w11tng::InterfaceStub::Delegate {
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};
    static constexpr unsigned int kConnectTimeout = 100;

    static mcs::NetworkManager::Ptr Create();

    ~NetworkManager();

    void SetDelegate(mcs::NetworkManager::Delegate * delegate) override;

    bool Setup() override;
    void Scan(const std::chrono::seconds &timeout) override;
    bool Connect(const mcs::NetworkDevice::Ptr &device) override;
    bool Disconnect(const mcs::NetworkDevice::Ptr &device) override;

    void SetWfdSubElements(const std::list<std::string> &elements) override;

    std::vector<mcs::NetworkDevice::Ptr> Devices() const override;
    mcs::IpV4Address LocalAddress() const override;
    bool Running() const override;
    bool Scanning() const override;

    void OnP2PDeviceChanged() override;
    void OnP2PDeviceReady() override;

    void OnDeviceFound(const std::string &path) override;
    void OnDeviceLost(const std::string &path) override;
    void OnGroupOwnerNegotiationFailure(const std::string &peer_path) override;
    void OnGroupOwnerNegotiationSuccess(const std::string &peer_path) override;
    void OnGroupStarted(const std::string &group_path, const std::string &interface_path, const std::string &role) override;
    void OnGroupFinished(const std::string &group_path, const std::string &interface_path) override;
    void OnGroupRequest(const std::string &peer_path) override;

    void OnDeviceChanged(const NetworkDevice::Ptr &device) override;
    void OnDeviceReady(const NetworkDevice::Ptr &device) override;

    void OnAddressAssigned(const mcs::IpV4Address &address) override;
    void OnNoLease() override;

    void OnFirmwareLoaded() override;
    void OnFirmwareUnloaded() override;

    void OnInterfaceSelectionDone(const std::string &path) override;

    void OnManagerReady() override;
    void OnManagerInterfaceAdded(const std::string &path) override;
    void OnManagerInterfaceRemoved(const std::string &path) override;

    void OnInterfaceReady() override;

private:
    static void OnServiceLost(GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void OnServiceFound(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data);

private:
    NetworkManager();
    std::shared_ptr<NetworkManager> FinalizeConstruction();

    NetworkDevice::Ptr FindDevice(const std::string &address);
    void Initialize(bool firmware_loading = false);
    void Release();
    void SetupInterface(const std::string &object_path);
    void AdvanceDeviceState(const NetworkDevice::Ptr &device, mcs::NetworkDeviceState state);

    void StartConnectTimeout();
    void StopConnectTimeout();

private:
    mcs::ScopedGObject<GDBusConnection> connection_;
    mcs::NetworkManager::Delegate *delegate_;
    std::shared_ptr<ManagerStub> manager_;
    std::shared_ptr<P2PDeviceStub> p2p_device_;
    std::unordered_map<std::string,w11tng::NetworkDevice::Ptr> devices_;
    NetworkDevice::Ptr current_device_;
    InterfaceStub::Ptr current_group_iface_;
    P2PDeviceStub::Ptr current_group_device_;
    std::shared_ptr<w11t::DhcpClient> dhcp_client_;
    std::shared_ptr<w11t::DhcpServer> dhcp_server_;
    InterfaceSelector::Ptr interface_selector_;
    guint connect_timeout_;
    w11t::WiFiFirmwareLoader firmware_loader_;
    bool has_dedicated_p2p_interface_;
};

} // namespace w11tng

#endif
