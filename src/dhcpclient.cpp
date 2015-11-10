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

#include <QDebug>

#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

#include "dhcpclient.h"
#include "networkutils.h"
#include "gdhcp.h"

class DhcpClient::Private
{
public:
    Private()
    {
    }

    static void onLeaseAvailable(GDHCPClient *client, gpointer user_data)
    {
        auto inst = static_cast<DhcpClient*>(user_data);

        inst->d->localAddress = g_dhcp_client_get_address(client);
        inst->d->netmask = g_dhcp_client_get_netmask(client);

        int index = NetworkUtils::retriveInterfaceIndex(inst->d->interface);

        if (NetworkUtils::modifyAddress(RTM_NEWADDR, NLM_F_REPLACE | NLM_F_ACK, index,
                                        AF_INET, inst->d->localAddress.toUtf8().constData(),
                                        NULL, 24, NULL) < 0) {
            qWarning() << "Failed to assign network address for" << inst->d->interface;
            return;
        }

        Q_EMIT inst->addressAssigned(inst->d->localAddress);
    }

    static void onClientDebug(const char *str, gpointer user_data)
    {
        qDebug() << "DHCP:" << str;
    }

    QString interface;
    GDHCPClient *client;
    QString localAddress;
    QString netmask;
};

DhcpClient::DhcpClient(const QString &interface) :
    d(new Private)
{
    d->interface = interface;
}

DhcpClient::~DhcpClient()
{
}

bool DhcpClient::start()
{
    int index = NetworkUtils::retriveInterfaceIndex(d->interface);
    if (index < 0)
        return false;

    GDHCPClientError error;
    d->client = g_dhcp_client_new(G_DHCP_IPV4, index, &error);
    if (!d->client) {
        qWarning() << "Failed to setup DHCP client";
        return false;
    }

    g_dhcp_client_set_debug(d->client, &Private::onClientDebug, this);

    g_dhcp_client_register_event(d->client, G_DHCP_CLIENT_EVENT_LEASE_AVAILABLE, &Private::onLeaseAvailable, this);

    g_dhcp_client_start(d->client, NULL);
}

void DhcpClient::stop()
{
    if (!d->client)
        return;

    g_dhcp_client_stop(d->client);

    g_dhcp_client_unref(d->client);

    d->localAddress = "";
    d->netmask = "";
}
