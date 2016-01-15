/*
 * Copyright (C) 2016 Canonical, Ltd.
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

#ifndef W11TNG_DHCP_LISTENER_STUB_H_
#define W11TNG_DHCP_LISTENER_STUB_H_

#include <gio/gio.h>

#include <memory>
#include <map>

#include <mcs/non_copyable.h>
#include <mcs/scoped_gobject.h>

namespace w11tng {
class DhcpListenerStub : public std::enable_shared_from_this<DhcpListenerStub> {
public:
    typedef std::shared_ptr<DhcpListenerStub> Ptr;

    static Ptr Create(const std::string &address);

    ~DhcpListenerStub();

    std::string Address() const;

    void EmitEvent(const std::map<std::string,std::string> &properties);

private:
    DhcpListenerStub(const std::string &address);
    Ptr FinalizeConstruction();

private:
    std::string address_;
    mcs::ScopedGObject<GDBusConnection> connection_;
};
} // namespace w11tng

#endif
