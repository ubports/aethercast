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

#ifndef DHCPCLIENT_H_
#define DHCPCLIENT_H_

#include <boost/noncopyable.hpp>

#include <string>

#include <mcs/ip_v4_address.h>
#include <mcs/non_copyable.h>

#include <w11t/gdhcp/gdhcp.h>

namespace w11t {
class DhcpClient {
public:
    class Delegate : private mcs::NonCopyable {
    public:
        virtual void OnAddressAssigned(const mcs::IpV4Address &address) = 0;

    protected:
        Delegate() = default;
    };

    DhcpClient(Delegate *delegate, const std::string &interface_name);
    ~DhcpClient();

    bool Start();
    void Stop();

    mcs::IpV4Address LocalAddress() const;

private:
    static void OnClientDebug(const char *str, gpointer user_data);
    static void OnLeaseAvailable(GDHCPClient *client, gpointer user_data);

private:
    Delegate *delegate_;
    std::string interface_name_;
    int interface_index_;
    GDHCPClient *client_;
    mcs::IpV4Address local_address_;
    std::string netmask_;
};
}

#endif
