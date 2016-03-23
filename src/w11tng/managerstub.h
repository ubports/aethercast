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

extern "C" {
// Ignore all warnings coming from the external headers as we don't
// control them and also don't want to get any warnings from them
// which will only polute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include "wpasupplicantinterface.h"
#pragma GCC diagnostic pop
}

namespace w11tng {

class ManagerStub : public std::enable_shared_from_this<ManagerStub> {
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};
    static constexpr const char *kManagerPath{"/fi/w1/wpa_supplicant1"};

    typedef std::shared_ptr<ManagerStub> Ptr;

    class Delegate : public mcs::NonCopyable {
    public:
        virtual void OnManagerReady() = 0;
        virtual void OnManagerInterfaceAdded(const std::string &path) = 0;
        virtual void OnManagerInterfaceRemoved(const std::string &path) = 0;
        virtual void OnManagerInterfaceCreationFailed() = 0;
    };

    static Ptr Create();

    ~ManagerStub();

    void SetDelegate(const std::weak_ptr<Delegate>& delegate);
    void ResetDelegate();

    std::vector<std::string> Capabilities() const;
    bool IsP2PSupported() const;
    std::vector<std::string> Interfaces() const;

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
    std::weak_ptr<Delegate> delegate_;
    mcs::ScopedGObject<GDBusConnection> connection_;
    mcs::ScopedGObject<WpaSupplicantFiW1Wpa_supplicant1> proxy_;
    bool p2p_supported_;
    std::vector<std::string> capabilities_;
    std::vector<std::string> interfaces_;
};

} // namespace w11tng

#endif
