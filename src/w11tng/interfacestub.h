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

#ifndef W11TNG_INTERFACE_STUB_H_
#define W11TNG_INTERFACE_STUB_H_

#include <vector>

#include <mcs/shared_gobject.h>
#include <mcs/scoped_gobject.h>
#include <mcs/non_copyable.h>
#include <mcs/mac_address.h>

#include <core/signal.h>

extern "C" {
#include "wpasupplicantinterface.h"
}

namespace w11tng {

class InterfaceStub : public std::enable_shared_from_this<InterfaceStub> {
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};

    typedef std::shared_ptr<InterfaceStub> Ptr;

    static Ptr Create(const std::string &object_path);

    ~InterfaceStub();

    std::vector<std::string> Capabilities() const;
    std::string Driver() const;
    std::string Ifname() const;

    const core::Signal<void>& InterfaceReady() const { return interface_ready_; }

private:
    InterfaceStub();
    Ptr FinalizeConstruction(const std::string &object_path);

private:
    mcs::ScopedGObject<GDBusConnection> connection_;
    mcs::ScopedGObject<WpaSupplicantInterface> proxy_;
    core::Signal<void> interface_ready_;
};

} // namespace w11tng

#endif
