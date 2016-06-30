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

#include "w11tng/urfkillmanager.h"

#include "ac/logger.h"
#include "ac/keep_alive.h"
#include "ac/dbus/helpers.h"

#include <string.h>
#include <errno.h>

namespace {
static constexpr int kInfiniteDbusWaitTimeout{-1};
}

namespace w11tng {
URfkillManager::Ptr URfkillManager::Create() {
    return std::shared_ptr<URfkillManager>(new URfkillManager)->FinalizeConstruction();
}

URfkillManager::URfkillManager()  {
}

URfkillManager::~URfkillManager() {
}

URfkillManager::Ptr URfkillManager::FinalizeConstruction() {
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
                                       &URfkillManager::OnPropertiesChanged,
                                       new ac::WeakKeepAlive<URfkillManager>(shared_from_this()),
                                       [](gpointer data) { delete static_cast<ac::WeakKeepAlive<URfkillManager>*>(data); });

    SyncProperties();

    return sp;
}

void URfkillManager::OnPropertiesChanged(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
                                        const gchar *interface_name, const gchar *signal_name, GVariant *parameters,
                                        gpointer user_data) {

    auto thiz = static_cast<ac::WeakKeepAlive<URfkillManager>*>(user_data)->GetInstance().lock();

    if (not thiz)
        return;

    thiz->ParseProperties(g_variant_get_child_value(parameters, 1));
}

void URfkillManager::SyncProperties() {
    g_dbus_connection_call(connection_.get(),
                           kBusName,
                           kObjectPath,
                           "org.freedesktop.DBus.Properties",
                           "GetAll",
                           g_variant_new ("(s)", "org.freedesktop.URfkill.Killswitch"),
                           G_VARIANT_TYPE("(a{sv})"),
                           G_DBUS_CALL_FLAGS_NONE,
                           kInfiniteDbusWaitTimeout, // Make sure we wait for the service being started up
                           nullptr,
                           [](GObject*, GAsyncResult *res, gpointer user_data) {
        auto thiz = static_cast<ac::SharedKeepAlive<URfkillManager>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        auto result = g_dbus_connection_call_finish(thiz->connection_.get(), res, &error);
        if (!result) {
            AC_ERROR("Failed to query urfkill service: %s", error->message);
            g_error_free(error);
            return;
        }

        thiz->ParseProperties(g_variant_get_child_value(result, 0));

    }, new ac::SharedKeepAlive<URfkillManager>{shared_from_this()});
}

void URfkillManager::ParseProperties(GVariant *properties) {
    ac::dbus::Helpers::ParseDictionary(properties, [&](const std::string &key, GVariant *value) {
        if (key == "state") {
            const auto state = g_variant_get_int32(g_variant_get_variant(value));
            const auto type = Type::kWLAN;
            block_status_[type] = (state != 0);
            if (auto sp = delegate_.lock())
                sp->OnRfkillChanged(type);
        }
    });
}

bool URfkillManager::IsBlocked(const Type &type) {
    auto it = block_status_.find(type);
    return it == block_status_.end() ? false : it->second;
}
} // namespace w11tng
