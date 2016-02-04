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

extern "C" {
#include "wpasupplicantinterface.h"
}

namespace w11tng {

class InterfaceStub : public std::enable_shared_from_this<InterfaceStub> {
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};

    typedef std::shared_ptr<InterfaceStub> Ptr;

    class Delegate : public mcs::NonCopyable {
    public:
        virtual void OnInterfaceReady() = 0;
    };

    static Ptr Create(const std::string &object_path);

    ~InterfaceStub();

    void SetDelegate(const std::weak_ptr<Delegate>& delegate);
    void ResetDelegate();

    std::vector<std::string> Capabilities() const;
    std::string Driver() const;
    std::string Ifname() const;
    std::string ObjectPath() const;

private:
    InterfaceStub();
    Ptr FinalizeConstruction(const std::string &object_path);

private:
    std::weak_ptr<Delegate> delegate_;
    mcs::ScopedGObject<GDBusConnection> connection_;
    mcs::ScopedGObject<WpaSupplicantInterface> proxy_;
};

} // namespace w11tng

#endif
