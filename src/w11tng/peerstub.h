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

#include <mcs/shared_gobject.h>
#include <mcs/scoped_gobject.h>
#include <mcs/non_copyable.h>
#include <mcs/mac_address.h>

extern "C" {
#include "wpasupplicantinterface.h"
}

namespace w11tng {

class PeerStub : public std::enable_shared_from_this<PeerStub> {
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};

    typedef std::shared_ptr<PeerStub> Ptr;

    class Delegate : public mcs::NonCopyable {
    public:
        virtual void OnPeerChanged() = 0;
        virtual void OnPeerReady() = 0;
    };

    static Ptr Create(const std::string &object_path);

    void SetDelegate(const std::weak_ptr<Delegate> delegate);
    void ResetDelegate();

    mcs::MacAddress Address() const;
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
    mcs::ScopedGObject<GDBusConnection> connection_;
    mcs::ScopedGObject<WpaSupplicantPeer> proxy_;
    std::weak_ptr<Delegate> delegate_;
    mcs::MacAddress address_;
    std::string name_;
};

} // namespace w11tng

#endif
