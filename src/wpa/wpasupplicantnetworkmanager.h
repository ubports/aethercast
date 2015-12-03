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

#include "wpasupplicantmessage.h"
#include "wpasupplicantcommandqueue.h"
#include "wififirmwareloader.h"

class WpaSupplicantNetworkManager : public mcs::NetworkManager,
                                    public WpaSupplicantCommandQueue::Delegate,
                                    public DhcpClient::Delegate,
                                    public wpa::WiFiFirmwareLoader::Delegate {
public:
    WpaSupplicantNetworkManager(mcs::NetworkManager::Delegate *delegate_);
    ~WpaSupplicantNetworkManager();

    bool Setup();

    void SetWfdSubElements(const std::list<std::string> &elements) override;

    void Scan(unsigned int timeout = 30) override;
    std::vector<mcs::NetworkDevice::Ptr> Devices() const override;

    bool Connect(const mcs::NetworkDevice::Ptr &device) override;
    bool DisconnectAll() override;

    mcs::IpV4Address LocalAddress() const override;

    bool Running() const override;

    void OnUnsolicitedResponse(WpaSupplicantMessage message);
    void OnWriteMessage(WpaSupplicantMessage message);

    void OnAddressAssigned(const mcs::IpV4Address &address);

    void OnFirmwareLoaded();
    void OnFirmwareUnloaded();

private:
    bool StartSupplicant();
    void StopSupplicant();
    bool ConnectSupplicant();
    void DisconnectSupplicant();
    void RequestAsync(const WpaSupplicantMessage &message, std::function<void(WpaSupplicantMessage)> callback);
    bool CreateSupplicantConfig(const std::string &conf_path);
    void HandleSupplicantFailed();
    void Reset();

    void OnP2pDeviceFound(WpaSupplicantMessage &message);
    void OnP2pDeviceLost(WpaSupplicantMessage &message);
    void OnP2pGroupStarted(WpaSupplicantMessage &message);
    void OnP2pGroupRemoved(WpaSupplicantMessage &message);

    static gboolean OnConnectSupplicant(gpointer user_data);
    static void OnSupplicantWatch(GPid pid, gint status, gpointer user_data);
    static gboolean OnGroupClientDhcpTimeout(gpointer user_data);
    static gboolean OnDeviceFailureTimeout(gpointer user_data);
    static gboolean OnIncomingMessages(GIOChannel *source, GIOCondition condition,
                                         gpointer user_data);
    static gboolean OnSupplicantRespawn(gpointer user_data);
    static void OnSupplicantProcessSetup(gpointer user_data);

private:
    NetworkManager::Delegate *delegate_;
    std::string interface_name_;
    wpa::WiFiFirmwareLoader firmware_loader_;
    std::string ctrl_path_;
    int sock_;
    std::map<std::string,mcs::NetworkDevice::Ptr> available_devices_;
    std::unique_ptr<WpaSupplicantCommandQueue> command_queue_;
    mcs::NetworkDevice::Ptr current_peer_;
    DhcpClient dhcp_client_;
    DhcpServer dhcp_server_;
    GPid supplicant_pid_;
    GIOChannel *channel_;
    guint channel_watch_;
    guint dhcp_timeout_;
    guint respawn_limit_;
    guint respawn_source_;
    bool is_group_owner_;
};

#endif
