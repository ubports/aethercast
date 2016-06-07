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

#include <ac/shared_gobject.h>
#include <ac/scoped_gobject.h>
#include <ac/non_copyable.h>
#include <ac/mac_address.h>

extern "C" {
// Ignore all warnings coming from the external headers as we don't
// control them and also don't want to get any warnings from them
// which will only pollute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include "wpasupplicantinterface.h"
#pragma GCC diagnostic pop
}

namespace w11tng {

class InterfaceStub : public std::enable_shared_from_this<InterfaceStub> {
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};

    typedef std::shared_ptr<InterfaceStub> Ptr;

    class Delegate : public ac::NonCopyable {
    public:
        virtual void OnInterfaceReady(const std::string &object_path) = 0;
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
    ac::ScopedGObject<GDBusConnection> connection_;
    ac::ScopedGObject<WpaSupplicantInterface> proxy_;
};

} // namespace w11tng

#endif
