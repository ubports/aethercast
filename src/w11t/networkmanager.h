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

#ifndef NETWORKP2PMANAGERWPASUPPLICANT_H_
#define NETWORKP2PMANAGERWPASUPPLICANT_H_

#include <functional>
#include <memory>
#include <map>

#include <glib.h>

#include <mcs/networkdevice.h>
#include <mcs/networkmanager.h>

#include "dhcpclient.h"
#include "dhcpserver.h"

#include "message.h"
#include "commandqueue.h"
#include "networkdevice.h"
#include "wififirmwareloader.h"

namespace w11t {
class NetworkManager : public mcs::NetworkManager,
                                    public CommandQueue::Delegate,
                                    public DhcpClient::Delegate,
                                    public w11t::WiFiFirmwareLoader::Delegate {
public:
    NetworkManager();
    ~NetworkManager();

    void SetDelegate(mcs::NetworkManager::Delegate *delegate) override;

    bool Setup() override;

    void SetWfdSubElements(const std::list<std::string> &elements) override;

    void Scan(const std::chrono::seconds &timeout) override;

    bool Connect(const mcs::NetworkDevice::Ptr &device) override;
    bool Disconnect(const mcs::NetworkDevice::Ptr &device) override;

    mcs::IpV4Address LocalAddress() const override;
    bool Running() const override;
    bool Scanning() const override;
    std::vector<mcs::NetworkDevice::Ptr> Devices() const override;

    void OnUnsolicitedResponse(Message message);
    void OnWriteMessage(Message message);

    void OnAddressAssigned(const mcs::IpV4Address &address);

    void OnFirmwareLoaded();
    void OnFirmwareUnloaded();

private:
    bool StartSupplicant();
    void StopSupplicant();
    bool ConnectSupplicant();
    void DisconnectSupplicant();
    void RequestAsync(const Message &message, std::function<void(Message)> callback = nullptr);
    bool CreateSupplicantConfig(const std::string &conf_path);
    void HandleSupplicantFailed();
    void Reset();
    void AdvanceDeviceState(const NetworkDevice::Ptr &device, mcs::NetworkDeviceState state);

    void OnP2pDeviceFound(Message &message);
    void OnP2pDeviceLost(Message &message);
    void OnP2pGroupStarted(Message &message);
    void OnP2pGroupRemoved(Message &message);
    void OnP2pGoNegFailure(Message &message);
    void OnP2pFindStopped(Message &message);
    void OnApStaConnected(Message &message);
    void OnApStaDisconnected(Message &message);

    static gboolean OnConnectSupplicant(gpointer user_data);
    static void OnSupplicantWatch(GPid pid, gint status, gpointer user_data);
    static gboolean OnGroupClientDhcpTimeout(gpointer user_data);
    static gboolean OnDeviceFailureTimeout(gpointer user_data);
    static gboolean OnIncomingMessages(GIOChannel *source, GIOCondition condition,
                                         gpointer user_data);
    static gboolean OnSupplicantRespawn(gpointer user_data);
    static void OnSupplicantProcessSetup(gpointer user_data);

private:
    mcs::NetworkManager::Delegate *delegate_;
    std::string interface_name_;
    w11t::WiFiFirmwareLoader firmware_loader_;
    std::string ctrl_path_;
    int sock_;
    std::map<std::string, NetworkDevice::Ptr> available_devices_;
    std::unique_ptr<CommandQueue> command_queue_;
    NetworkDevice::Ptr current_peer_;
    DhcpClient dhcp_client_;
    DhcpServer dhcp_server_;
    GPid supplicant_pid_;
    GIOChannel *channel_;
    guint channel_watch_;
    guint dhcp_timeout_;
    guint respawn_limit_;
    guint respawn_source_;
    bool is_group_owner_;
    bool scanning_;
};
}
#endif
