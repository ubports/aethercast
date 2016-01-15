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

#ifndef W11TNG_DHCPSERVER_H_
#define W11TNG_DHCPSERVER_H_

#include <glib.h>

#include <boost/noncopyable.hpp>

#include <string>

#include <mcs/ip_v4_address.h>
#include <mcs/non_copyable.h>

namespace w11tng {
class DhcpServer : public std::enable_shared_from_this<DhcpServer> {
public:
    typedef std::shared_ptr<DhcpServer> Ptr;

    class Delegate : private mcs::NonCopyable {
    public:
        virtual void OnLeaseAdded() = 0;

    protected:
        Delegate() = default;
    };

    static Ptr Create(Delegate *delegate, const std::string &interface_name);

    ~DhcpServer();

    bool Start();
    void Stop();

    bool Running() const { return pid_ > 0; }
    mcs::IpV4Address LocalAddress() const;

private:
    DhcpServer(Delegate *delegate, const std::string &interface_name);

private:
    std::string interface_name_;
    GPid pid_;
    std::string lease_file_path_;
};
}
#endif
