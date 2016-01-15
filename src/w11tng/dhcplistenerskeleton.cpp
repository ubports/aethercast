/*
 * Copyright (C) 2016 Canonical, Ltd.
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

#include <mcs/logger.h>
#include <mcs/utils.h>
#include <mcs/keep_alive.h>

#include <w11tng/config.h>

#include "dhcplistenerskeleton.h"

namespace w11tng {

DhcpListenerSkeleton::Ptr DhcpListenerSkeleton::Create(const std::string &path, std::weak_ptr<Delegate> delegate) {
    return std::shared_ptr<DhcpListenerSkeleton>(new DhcpListenerSkeleton(delegate))->FinalizeConstruction(path);
}

DhcpListenerSkeleton::DhcpListenerSkeleton(std::weak_ptr<Delegate> delegate) :
    delegate_(delegate) {
}

DhcpListenerSkeleton::~DhcpListenerSkeleton() {
}

DhcpListenerSkeleton::Ptr DhcpListenerSkeleton::FinalizeConstruction(const std::string &path) {
    auto sp = shared_from_this();

    ::unlink(path.c_str());

    address_ = mcs::Utils::Sprintf("unix:path=%s", path);
    auto guid = g_dbus_generate_guid();

    auth_observer_.reset(g_dbus_auth_observer_new());

    g_signal_connect_data(auth_observer_.get(), "authorize-authenticated-peer", G_CALLBACK(OnAuthorizePeer), new mcs::WeakKeepAlive<DhcpListenerSkeleton>(sp),
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<DhcpListenerSkeleton>*>(data); }, GConnectFlags(0));

    GError *error = nullptr;
    server_.reset(g_dbus_server_new_sync(address_.c_str(), G_DBUS_SERVER_FLAGS_NONE,
                                     guid, auth_observer_.get(), nullptr, &error));
    g_free(guid);
    if (!server_) {
        MCS_WARNING("Failed to setup private dbus server to communicate with DHCP client: %s",
                    error->message);
        g_error_free(error);
        return sp;
    }

    g_signal_connect_data(server_.get(), "new-connection", G_CALLBACK(OnNewConnection), new mcs::WeakKeepAlive<DhcpListenerSkeleton>(sp),
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<DhcpListenerSkeleton>*>(data); }, GConnectFlags(0));

    return sp;
}

std::string DhcpListenerSkeleton::Address() const {
    return address_;
}

void DhcpListenerSkeleton::ConnectSignals() {
    if (!current_connection_)
        return;

    g_signal_connect_data(current_connection_.get(), "closed", G_CALLBACK(OnConnectionClosed), new mcs::WeakKeepAlive<DhcpListenerSkeleton>(shared_from_this()),
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<DhcpListenerSkeleton>*>(data); }, GConnectFlags(0));

    g_dbus_connection_signal_subscribe(current_connection_.get(), nullptr, "org.aethercast.dhcp_client", "Event",
                                       nullptr, nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
                                       &DhcpListenerSkeleton::OnEvent, new mcs::WeakKeepAlive<DhcpListenerSkeleton>(shared_from_this()),
                                       [](gpointer data) { delete static_cast<mcs::WeakKeepAlive<DhcpListenerSkeleton>*>(data); });
}

gboolean DhcpListenerSkeleton::OnAuthorizePeer(GDBusAuthObserver *observer, GIOStream *stream,
                                               GCredentials *credentials, gpointer user_data) {

    auto thiz = static_cast<mcs::WeakKeepAlive<DhcpListenerSkeleton>*>(user_data)->GetInstance().lock();
    if (not thiz)
        return FALSE;

    MCS_DEBUG("");

    // We only allow one connection
    if (thiz->current_connection_)
        return FALSE;

    // Only root is allowed to connect, all others are forbidden
    return g_credentials_get_unix_user(credentials, nullptr) == 0;
}

gboolean DhcpListenerSkeleton::OnNewConnection(GDBusServer *server, GDBusConnection *connection,
                                               gpointer user_data)  {

    auto thiz = static_cast<mcs::WeakKeepAlive<DhcpListenerSkeleton>*>(user_data)->GetInstance().lock();
    if (not thiz)
        return FALSE;

    MCS_DEBUG("");

    g_object_ref(connection);
    thiz->current_connection_.reset(connection);

    thiz->ConnectSignals();

    if (auto sp = thiz->delegate_.lock())
        sp->OnNewConnection();

    return TRUE;
}

void DhcpListenerSkeleton::OnConnectionClosed(GDBusConnection *connection, gboolean remote_peer_vanished,
                                              GError *error, gpointer user_data) {

    auto thiz = static_cast<mcs::WeakKeepAlive<DhcpListenerSkeleton>*>(user_data)->GetInstance().lock();
    if (not thiz || !thiz->current_connection_)
        return;

    MCS_DEBUG("");

    thiz->current_connection_.reset();

    if (auto sp = thiz->delegate_.lock())
        sp->OnConnectionClosed();
}

void DhcpListenerSkeleton::OnEvent(GDBusConnection *connection, const char *sender_name, const char *object_path,
                                   const char *interface_name, const char *signal_name, GVariant *parameters,
                                   gpointer user_data) {

    auto thiz = static_cast<mcs::WeakKeepAlive<DhcpListenerSkeleton>*>(user_data)->GetInstance().lock();
    if (not thiz)
        return;

    MCS_DEBUG("");

    std::map<std::string,std::string> properties;

    if (!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(a{sv})"))) {
        MCS_DEBUG("Got event with invalid value type %s", g_variant_print(parameters, TRUE));
        return;
    }

    GVariant *props = nullptr;
    g_variant_get(parameters, "(@a{sv})", &props);

    GVariantIter iter;
    g_variant_iter_init (&iter, props);

    const char *key;
    GVariant *value;
    while (g_variant_iter_loop (&iter, "{sv}", &key, &value))
        properties[std::string(key)] = g_variant_get_string(value, nullptr);

    if (auto sp = thiz->delegate_.lock())
        sp->OnEvent(properties);
}

} // namespace w11tng
