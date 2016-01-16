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

#ifndef W11TNG_DHCPCLIENT_H_
#define W11TNG_DHCPCLIENT_H_

#include <glib.h>

#include <boost/noncopyable.hpp>

#include <string>

#include <mcs/ip_v4_address.h>
#include <mcs/non_copyable.h>

#include "netlinklistener.h"

namespace w11tng {
class DhcpClient : public std::enable_shared_from_this<DhcpClient>,
                   public w11tng::NetlinkListener::Delegate {
public:
    typedef std::shared_ptr<DhcpClient> Ptr;

    class Delegate : private mcs::NonCopyable {
    public:
        virtual void OnAddressAssigned(const mcs::IpV4Address &address) = 0;
        virtual void OnNoLease() = 0;

    protected:
        Delegate() = default;
    };

    static Ptr Create(const std::weak_ptr<Delegate> &delegate, const std::string &interface_name);

    DhcpClient(const std::weak_ptr<Delegate> &delegate, const std::string &interface_name);
    ~DhcpClient();

    bool Start();
    void Stop();

    mcs::IpV4Address LocalAddress() const;

    void OnInterfaceAddressChanged(const std::string &interface, const std::string &address);

private:
    std::weak_ptr<Delegate> delegate_;
    std::string interface_name_;
    w11tng::NetlinkListener::Ptr netlink_listener_;
    mcs::IpV4Address local_address_;
    GPid pid_;
    guint process_watch_;
};
}

#endif
