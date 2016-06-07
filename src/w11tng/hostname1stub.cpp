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

#include <boost/concept_check.hpp>

#include <ac/keep_alive.h>
#include <ac/dbus/helpers.h>

#include "hostname1stub.h"

namespace w11tng {

Hostname1Stub::Ptr Hostname1Stub::Create(const std::weak_ptr<Delegate> &delegate) {
    return std::shared_ptr<Hostname1Stub>(new Hostname1Stub(delegate))->FinalizeConstruction();
}

Hostname1Stub::Ptr Hostname1Stub::FinalizeConstruction() {
    auto sp = shared_from_this();

    GError *error = nullptr;
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!connection_) {
        AC_ERROR("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    g_dbus_connection_signal_subscribe(connection_.get(),
                                       kBusName,
                                       "org.freedesktop.DBus.Properties",
                                       "PropertiesChanged",
                                       kObjectPath,
                                       nullptr,
                                       G_DBUS_SIGNAL_FLAGS_NONE,
                                       &Hostname1Stub::OnPropertiesChanged,
                                       new ac::WeakKeepAlive<Hostname1Stub>(shared_from_this()),
                                       [](gpointer data) { delete static_cast<ac::WeakKeepAlive<Hostname1Stub>*>(data); });

    SyncProperties();

    return sp;
}

Hostname1Stub::Hostname1Stub(const std::weak_ptr<Delegate> &delegate) :
    delegate_(delegate) {
}

Hostname1Stub::~Hostname1Stub() {
}

void Hostname1Stub::OnPropertiesChanged(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
                                        const gchar *interface_name, const gchar *signal_name, GVariant *parameters,
                                        gpointer user_data) {

    auto thiz = static_cast<ac::WeakKeepAlive<Hostname1Stub>*>(user_data)->GetInstance().lock();

    if (not thiz)
        return;

    thiz->ParseProperties(parameters);

    if (auto sp = thiz->delegate_.lock())
        sp->OnHostnameChanged();
}

void Hostname1Stub::SyncProperties() {
    g_dbus_connection_call(connection_.get(),
                           kBusName,
                           kObjectPath,
                           "org.freedesktop.DBus.Properties",
                           "GetAll",
                           g_variant_new ("(s)", "org.freedesktop.hostname1"),
                           G_VARIANT_TYPE("(a{sv})"),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1, // Make sure we wait for the service being started up
                           nullptr,
                           [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto thiz = static_cast<ac::SharedKeepAlive<Hostname1Stub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        auto result = g_dbus_connection_call_finish(thiz->connection_.get(), res, &error);
        if (!result) {
            AC_ERROR("Failed to query hostname service: %s", error->message);
            g_error_free(error);
            return;
        }

        thiz->ParseProperties(g_variant_get_child_value(result, 0));

    }, new ac::SharedKeepAlive<Hostname1Stub>{shared_from_this()});
}

void Hostname1Stub::ParseProperties(GVariant *properties) {
    ac::dbus::Helpers::ParseDictionary(properties, [&](const std::string &key, GVariant *value) {
        if (key == "Hostname")
            hostname_ = g_variant_get_string(g_variant_get_variant(value), nullptr) ? : "";
        else if (key == "StaticHostname")
            static_hostname_ = g_variant_get_string(g_variant_get_variant(value), nullptr) ? : "";
        else if (key == "PrettyHostname")
            pretty_hostname_ = g_variant_get_string(g_variant_get_variant(value), nullptr) ? : "";
        else if (key == "Chassis")
            chassis_ = g_variant_get_string(g_variant_get_variant(value), nullptr) ? : "";
    });
}

std::string Hostname1Stub::Hostname() const {
    return hostname_;
}

std::string Hostname1Stub::StaticHostname() const {
    return static_hostname_;
}

std::string Hostname1Stub::PrettyHostname() const {
    return pretty_hostname_;
}

std::string Hostname1Stub::Chassis() const {
    return chassis_;
}
} // namespace w11tng
