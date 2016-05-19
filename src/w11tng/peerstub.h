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

#ifndef W11TNG_PEER_STUB_H_
#define W11TNG_PEER_STUB_H_

#include <string>

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

class PeerStub : public std::enable_shared_from_this<PeerStub> {
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};

    typedef std::shared_ptr<PeerStub> Ptr;

    class Delegate : public ac::NonCopyable {
    public:
        virtual void OnPeerChanged() = 0;
        virtual void OnPeerReady() = 0;
    };

    static Ptr Create(const std::string &object_path);

    void SetDelegate(const std::weak_ptr<Delegate> &delegate);
    void ResetDelegate();

    ac::MacAddress Address() const;
    std::string Name() const;

private:
    PeerStub() { }

    Ptr FinalizeConstruction(const std::string &object_path);

    void ConnectSignals();
    void SyncProperties(bool update_delegate = true);

    std::string RetrieveAddressFromProxy();

private:
    static void OnPropertyChanged(GObject *source, GParamSpec *spec, gpointer user_data);

private:
    ac::ScopedGObject<GDBusConnection> connection_;
    ac::ScopedGObject<WpaSupplicantPeer> proxy_;
    std::weak_ptr<Delegate> delegate_;
    ac::MacAddress address_;
    std::string name_;
};

} // namespace w11tng

#endif
