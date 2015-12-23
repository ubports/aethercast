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

#include <mcs/keep_alive.h>
#include <mcs/logger.h>

#include "peerstub.h"

namespace w11tng {

PeerStub::Ptr PeerStub::Create(const std::string &object_path) {
    return std::shared_ptr<PeerStub>(new PeerStub)->FinalizeConstruction(object_path);
}

PeerStub::Ptr PeerStub::FinalizeConstruction(const std::string &object_path) {
    auto sp = shared_from_this();

    GError *error = nullptr;
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!connection_) {
        MCS_ERROR("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    wpa_supplicant_peer_proxy_new(connection_.get(), G_DBUS_PROXY_FLAGS_NONE,
                                  kBusName,
                                  object_path.c_str(),
                                  nullptr,
                                  [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<PeerStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        inst->proxy_.reset(wpa_supplicant_peer_proxy_new_finish(res, &error));
        if (!inst->proxy_) {
            MCS_ERROR("Failed to connect with Peer proxy: %s", error->message);
            g_error_free(error);
            return;
        }

        MCS_DEBUG("Connected with peer");

        inst->ConnectSignals();
        inst->SyncProperties();

    }, new mcs::SharedKeepAlive<PeerStub>{shared_from_this()});

    return sp;
}

void PeerStub::OnPropertyChanged(GObject *source, GParamSpec *spec, gpointer user_data) {
    auto inst = static_cast<mcs::SharedKeepAlive<PeerStub>*>(user_data)->ShouldDie();

    if (not inst)
        return;

    MCS_DEBUG("Peer property changed");

    inst->SyncProperties();
}

void PeerStub::ConnectSignals() {
    g_signal_connect_data(proxy_.get(), "notify::device-name",
                          G_CALLBACK(&PeerStub::OnPropertyChanged), new mcs::WeakKeepAlive<PeerStub>(shared_from_this()),
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<PeerStub>*>(data); }, GConnectFlags(0));
}

void PeerStub::SyncProperties() {
    if (!proxy_)
        return;

    address_ = wpa_supplicant_peer_get_device_address(proxy_.get());
    name_ = wpa_supplicant_peer_get_device_name(proxy_.get());

    if (auto sp = delegate_.lock())
        sp->OnChanged();
}

void PeerStub::SetDelegate(const std::weak_ptr<Delegate> delegate) {
    delegate_ = delegate;
}

void PeerStub::ResetDelegate() {
    delegate_.reset();
}

mcs::MacAddress PeerStub::Address() const {
    return address_;
}

std::string PeerStub::Name() const {
    return name_;
}

} // namespace w11tng
