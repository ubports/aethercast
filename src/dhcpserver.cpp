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

#include "dhcpserver.h"
#include "networkutils.h"
#include "gdhcp.h"

class DhcpServer::Private
{
public:
    static void onServerDebug(const char *str, gpointer user_data)
    {
        qDebug() << "DHCP:" << str;
    }

    QString interface;
    GDHCPServer *server;
};

DhcpServer::DhcpServer(const QString &interface) :
    d(new Private)
{
    d->interface = interface;
}

DhcpServer::~DhcpServer()
{
    if (d->server)
        g_dhcp_server_unref(d->server);
}

QString DhcpServer::localAddress() const
{
    // FIXME this should be stored somewhere else
    return QString("192.168.7.1");
}

bool DhcpServer::start()
{
    qWarning() << "Starting up DHCP server";

    int index = NetworkUtils::retriveInterfaceIndex(d->interface);
    if (index < 0) {
        qWarning() << "Failed to determine index of network interface" << d->interface;
        return false;
    }

    // FIXME store those defaults somewhere else
    const char *address = "192.168.7.1";
    const char *subnet = "255.255.255.0";
    const char *broadcast = "192.168.7.255";
    unsigned char prefixlen = 24;

    if (NetworkUtils::modifyAddress(RTM_NEWADDR, NLM_F_REPLACE | NLM_F_ACK, index,
                                    AF_INET, address,
                                    NULL, prefixlen, broadcast) < 0) {
        qWarning() << "Failed to assign network address for" << d->interface;
        return false;
    }

    GDHCPServerError error;
    d->server = g_dhcp_server_new(G_DHCP_IPV4, index, &error);
    if (!d->server) {
        qWarning() << "Failed to setup DHCP server";
        return false;
    }

    g_dhcp_server_set_lease_time(d->server, 3600);
    g_dhcp_server_set_option(d->server, G_DHCP_SUBNET, subnet);
    g_dhcp_server_set_option(d->server, G_DHCP_ROUTER, localAddress().toUtf8().constData());
    g_dhcp_server_set_option(d->server, G_DHCP_DNS_SERVER, NULL);
    g_dhcp_server_set_ip_range(d->server, "192.168.7.5", "192.168.7.100");

    g_dhcp_server_set_debug(d->server, &Private::onServerDebug, this);

    if(g_dhcp_server_start(d->server) < 0) {
        qWarning() << "Failed to start DHCP server";
        g_dhcp_server_unref(d->server);
        return false;
    }

    return true;
}

void DhcpServer::stop()
{
    if (!d->server)
        return;

    g_dhcp_server_stop(d->server);

    g_dhcp_server_unref(d->server);
}
