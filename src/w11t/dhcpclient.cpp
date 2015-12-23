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

#include "dhcpclient.h"
#include "gdhcp.h"

namespace w11t {
DhcpClient::DhcpClient(Delegate *delegate, const std::string &interface_name) :
    delegate_(delegate),
    interface_name_(interface_name),
    client_(nullptr) {
    interface_index_ = mcs::NetworkUtils::RetrieveInterfaceIndex(interface_name_.c_str());
    if (interface_index_ < 0)
        MCS_WARNING("Failed to determine index of network interface %s", interface_name_.c_str());
}

DhcpClient::~DhcpClient() {
}

void DhcpClient::OnLeaseAvailable(GDHCPClient *client, gpointer user_data) {
    auto inst = static_cast<DhcpClient*>(user_data);

    char *address = g_dhcp_client_get_address(inst->client_);
    char *netmask = g_dhcp_client_get_netmask(inst->client_);

    if (!address) {
        MCS_WARNING("Received invalid IP configuration over DHCP");
        return;
    }

    if (mcs::NetworkUtils::ModifyInterfaceAddress(RTM_NEWADDR, NLM_F_REPLACE | NLM_F_ACK, inst->interface_index_,
                                    AF_INET, address,
                                    NULL, 24, NULL) < 0) {
        MCS_WARNING("Failed to assign network address for %s", inst->interface_name_.c_str());
        return;
    }

    inst->local_address_ = mcs::IpV4Address::from_string(address);

    if (netmask)
        inst->netmask_.assign(netmask);

    inst->delegate_->OnAddressAssigned(inst->local_address_);
}

void DhcpClient::OnClientDebug(const char *str, gpointer user_data) {
    MCS_DEBUG("DHCP: %s", str);
}

mcs::IpV4Address DhcpClient::LocalAddress() const {
    return local_address_;
}

bool DhcpClient::Start() {
    GDHCPClientError error;
    client_ = g_dhcp_client_new(G_DHCP_IPV4, interface_index_, &error);
    if (!client_) {
        MCS_ERROR("Failed to setup DHCP client: %s", g_dhcp_client_error_to_string(error));
        return false;
    }

    g_dhcp_client_set_debug(client_, &DhcpClient::OnClientDebug, this);
    g_dhcp_client_register_event(client_, G_DHCP_CLIENT_EVENT_LEASE_AVAILABLE, &DhcpClient::OnLeaseAvailable, this);
    g_dhcp_client_start(client_, NULL);

    return true;
}

void DhcpClient::Stop() {
    if (!client_)
        return;

    g_dhcp_client_stop(client_);
    g_dhcp_client_unref(client_);

    local_address_ = mcs::IpV4Address{};
    netmask_ = "";

    mcs::NetworkUtils::ResetInterface(interface_index_);
}
}
