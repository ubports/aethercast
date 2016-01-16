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

#ifndef W11TNG_NETLINK_LISTENER_H_
#define W11TNG_NETLINK_LISTENER_H_

#include <glib.h>

#include <memory>

#include <mcs/non_copyable.h>
#include <mcs/scoped_gobject.h>

struct nlmsghdr;

namespace w11tng {

class NetlinkListener : public std::enable_shared_from_this<NetlinkListener> {
public:
    typedef std::shared_ptr<NetlinkListener> Ptr;

    class Delegate : public mcs::NonCopyable {
    public:
        virtual void OnInterfaceAddressChanged(const std::string &interface, const std::string &address) { }
    };

    static Ptr Create(const std::weak_ptr<Delegate> &delegate);

    ~NetlinkListener();

    void SetInterfaceFilter(const std::string &interface_name);

private:
    NetlinkListener(const std::weak_ptr<Delegate> &delegate);
    Ptr FinalizeConstruction();

    void ProcessNewAddress(struct nlmsghdr *hdr);

private:
    static gboolean OnDataAvailable(GIOChannel *channel, GIOCondition condition, gpointer user_data);

private:
    const std::weak_ptr<Delegate> delegate_;
    GIOChannel *channel_;
    int interface_index_filter_;
};

} // namespace w11tng

#endif
