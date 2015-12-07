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

#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

#include <mcs/logger.h>
#include <mcs/networkutils.h>

#include "dhcpserver.h"

DhcpServer::DhcpServer(Delegate *delegate, const std::string &interface_name) :
    interface_name_(interface_name) {
    interface_index_ = mcs::NetworkUtils::RetrieveInterfaceIndex(interface_name_.c_str());
    if (interface_index_ < 0)
        MCS_WARNING("Failed to determine index of network interface: %s", interface_name_.c_str());
}

DhcpServer::~DhcpServer()
{
    if (server_)
        g_dhcp_server_unref(server_);
}

mcs::IpV4Address DhcpServer::LocalAddress() const
{
    // FIXME this should be stored somewhere else
    return mcs::IpV4Address::from_string("192.168.7.1");
}

void DhcpServer::OnDebug(const char *str, gpointer user_data)
{
    MCS_WARNING("DHCP: %s", str);
}

bool DhcpServer::Start()
{
    MCS_WARNING("Starting up DHCP server");

    // FIXME store those defaults somewhere else
    const char *address = "192.168.7.1";
    const char *subnet = "255.255.255.0";
    const char *broadcast = "192.168.7.255";
    unsigned char prefixlen = 24;

    if (mcs::NetworkUtils::ModifyInterfaceAddress(RTM_NEWADDR, NLM_F_REPLACE | NLM_F_ACK, interface_index_,
                                    AF_INET, address,
                                    NULL, prefixlen, broadcast) < 0) {
        MCS_WARNING("Failed to assign network address for %s", interface_name_.c_str());
        return false;
    }

    GDHCPServerError error;
    server_ = g_dhcp_server_new(G_DHCP_IPV4, interface_index_, &error);
    if (!server_) {
        MCS_WARNING("Failed to setup DHCP server");
        return false;
    }

    g_dhcp_server_set_lease_time(server_, 3600);
    g_dhcp_server_set_option(server_, G_DHCP_SUBNET, subnet);
    g_dhcp_server_set_option(server_, G_DHCP_ROUTER, LocalAddress().to_string().c_str());
    g_dhcp_server_set_option(server_, G_DHCP_DNS_SERVER, NULL);
    g_dhcp_server_set_ip_range(server_, "192.168.7.5", "192.168.7.100");

    g_dhcp_server_set_debug(server_, &DhcpServer::OnDebug, this);

    if(g_dhcp_server_start(server_) < 0) {
        MCS_WARNING("Failed to start DHCP server");
        g_dhcp_server_unref(server_);
        return false;
    }

    return true;
}

void DhcpServer::Stop()
{
    if (!server_)
        return;

    g_dhcp_server_stop(server_);
    g_dhcp_server_unref(server_);

    mcs::NetworkUtils::ResetInterface(interface_index_);
}
