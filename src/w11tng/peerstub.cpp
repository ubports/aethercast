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

#include <sstream>

#include <mcs/keep_alive.h>
#include <mcs/logger.h>
#include <mcs/utils.h>

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

        inst->ConnectSignals();
        inst->SyncProperties(false);

        if (auto sp = inst->delegate_.lock())
            sp->OnPeerReady();

    }, new mcs::SharedKeepAlive<PeerStub>{shared_from_this()});

    return sp;
}

void PeerStub::OnPropertyChanged(GObject *source, GParamSpec *spec, gpointer user_data) {
    auto inst = static_cast<mcs::SharedKeepAlive<PeerStub>*>(user_data)->ShouldDie();

    if (not inst)
        return;

    inst->SyncProperties();
}

void PeerStub::ConnectSignals() {
    g_signal_connect_data(proxy_.get(), "notify::device-name",
                          G_CALLBACK(&PeerStub::OnPropertyChanged), new mcs::WeakKeepAlive<PeerStub>(shared_from_this()),
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<PeerStub>*>(data); },
                          GConnectFlags(0));

    g_signal_connect_data(proxy_.get(), "notify::device-address",
                          G_CALLBACK(&PeerStub::OnPropertyChanged), new mcs::WeakKeepAlive<PeerStub>(shared_from_this()),
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<PeerStub>*>(data); },
                          GConnectFlags(0));

}

std::string ByteArrayToMacAddress(const gchar *data) {
    if (!data)
        return "";

    std::stringstream ss;
    for (int n = 0; n < 6; n++) {
        ss << mcs::Utils::Sprintf("%02x", (uint16_t) (data[n] & 0xff));
        //ss << buf;
        if (n < 5)
            ss << ":";
    }
    return ss.str();
}

std::string PeerStub::RetrieveAddressFromProxy() {
    // NOTE We have to parse the 'DeviceAddress' property here manually as
    // gdbus-codegen doesn't properly give us a value of type 'ay'. Or it
    // could be still that I am to stupid using it but this now works
    // perfectly.

    WpaSupplicantPeerProxy *proxy = WPA_SUPPLICANT_PEER_PROXY(proxy_.get());
    auto variant = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (proxy), "DeviceAddress");

    if (!variant)
        return "";

    GVariantIter iter;
    gchar raw_addr[6];

    g_variant_iter_init(&iter, variant);

    for (int n = 0; n < 6; n++) {
        if (!g_variant_iter_next(&iter, "y", &raw_addr[n]))
            break;
    }

    auto address = ByteArrayToMacAddress(raw_addr);

    g_variant_unref (variant);

    return address;
}

void PeerStub::SyncProperties(bool update_delegate) {
    if (!proxy_)
        return;

    name_ = wpa_supplicant_peer_get_device_name(proxy_.get());
    address_ = RetrieveAddressFromProxy();

    if (!update_delegate)
        return;

    if (auto sp = delegate_.lock())
        sp->OnPeerChanged();
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
