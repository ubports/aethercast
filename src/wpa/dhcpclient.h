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

#include <boost/core/noncopyable.hpp>

#include <string>

#include <gdhcp.h>

class DhcpClient {
public:
    class Delegate : private boost::noncopyable {
    public:
        virtual void OnAddressAssigned(const std::string &address) = 0;

    protected:
        Delegate() = default;
    };

    DhcpClient(Delegate *delegate, const std::string &interface_name);
    ~DhcpClient();

    bool Start();
    void Stop();

    std::string LocalAddress() const;

private:
    static void OnClientDebug(const char *str, gpointer user_data);
    static void OnLeaseAvailable(GDHCPClient *client, gpointer user_data);

private:
    Delegate *delegate_;
    std::string interface_name_;
    int interface_index_;
    GDHCPClient *client_;
    std::string local_address_;
    std::string netmask_;
};

#endif
