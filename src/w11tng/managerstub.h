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

#ifndef W11TNG_MANAGER_STUB_H_
#define W11TNG_MANAGER_STUB_H_

#include <vector>

#include <mcs/shared_gobject.h>
#include <mcs/scoped_gobject.h>

#include <core/signal.h>

extern "C" {
#include "wpasupplicantinterface.h"
}

namespace w11tng {

class ManagerStub : public std::enable_shared_from_this<ManagerStub> {
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};
    static constexpr const char *kManagerPath{"/fi/w1/wpa_supplicant1"};

    typedef std::shared_ptr<ManagerStub> Ptr;

    static Ptr Create();

    ~ManagerStub();

    std::vector<std::string> Capabilities() const;
    bool IsP2PSupported() const;
    std::vector<std::string> Interfaces() const;

    const core::Signal<void>& Ready() const { return ready_; }
    const core::Signal<std::string>& InterfaceAdded() const { return interface_added_; }
    const core::Signal<std::string>& InterfaceRemoved() const { return interface_removed_; }

    void SetWFDIEs(uint8_t *bytes, int length);

    void CreateInterface(const std::string &interface_name);

private:
    ManagerStub();
    Ptr FinalizeConstruction();

    void Initialize();

private:
    static void OnInterfaceAdded(GObject *source, const gchar *path, GVariant *properties, gpointer user_data);
    static void OnInterfaceRemoved(GObject *source, const gchar *path, gpointer user_data);

private:
    mcs::ScopedGObject<GDBusConnection> connection_;
    mcs::ScopedGObject<WpaSupplicantFiW1Wpa_supplicant1> proxy_;
    core::Signal<void> ready_;
    bool p2p_supported_;
    std::vector<std::string> capabilities_;
    std::vector<std::string> interfaces_;
    core::Signal<std::string> interface_added_;
    core::Signal<std::string> interface_removed_;
};

} // namespace w11tng

#endif
