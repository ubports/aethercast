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

#ifndef W11TNG_INTERFACE_SELECTOR_H_
#define W11TNG_INTERFACE_SELECTOR_H_

#include <memory>
#include <vector>

#include <gio/gio.h>

#include <mcs/scoped_gobject.h>

#include <core/signal.h>

namespace w11tng {

typedef std::vector<std::string> InterfaceList;

class InterfaceSelector : public std::enable_shared_from_this<InterfaceSelector> {
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};

    typedef std::shared_ptr<InterfaceSelector> Ptr;

    static Ptr Create();

    ~InterfaceSelector();

    void Process(const InterfaceList &interfaces);

    const core::Signal<std::string>& Done() const { return done_; }

private:
    InterfaceSelector();
    Ptr FinalizeConstruction();

    void TryNextInterface();

private:
    mcs::ScopedGObject<GDBusConnection> connection_;
    std::vector<std::string> interfaces_;
    core::Signal<std::string> done_;
};

} // namespace w11tng

#endif
