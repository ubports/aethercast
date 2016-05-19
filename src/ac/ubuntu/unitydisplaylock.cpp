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

#include "ac/logger.h"
#include "ac/keep_alive.h"

#include "ac/ubuntu/unity.h"
#include "ac/ubuntu/unitydisplaylock.h"

namespace ac {
namespace ubuntu {

UnityDisplayLock::Ptr UnityDisplayLock::Create() {
    return std::shared_ptr<UnityDisplayLock>(new UnityDisplayLock);
}

UnityDisplayLock::UnityDisplayLock() :
    ref_count_(0),
    cookie_(0) {

    GError *error = nullptr;
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!connection_) {
        ERROR("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return;
    }
}

UnityDisplayLock::~UnityDisplayLock() {
    ReleaseInternal();
}

void UnityDisplayLock::Acquire(DisplayState state) {
    if (ref_count_ > 0) {
        ref_count_++;
        return;
    }

    g_dbus_connection_call(connection_.get(),
                           unity::screen::kBusName,
                           unity::screen::kPath,
                           unity::screen::kInterfaceName,
                           "keepDisplayOn",
                           nullptr,
                           G_VARIANT_TYPE("(i)"),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1, // Make sure we wait for the service being started up
                           nullptr,
                           [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto thiz = static_cast<ac::SharedKeepAlive<UnityDisplayLock>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        auto result = g_dbus_connection_call_finish(thiz->connection_.get(), res, &error);
        if (!result) {
            ERROR("Failed to acquire display lock: %s", error->message);
            g_error_free(error);
            return;
        }

        thiz->cookie_ = g_variant_get_int32(g_variant_get_child_value(result, 0));
        thiz->ref_count_ = 1;

        DEBUG("Successfully locked display (cookie %d)", thiz->cookie_);

    }, new ac::SharedKeepAlive<UnityDisplayLock>{shared_from_this()});
}

void UnityDisplayLock::ReleaseInternal() {
    if (ref_count_ == 0)
        return;

    g_dbus_connection_call(connection_.get(),
                           unity::screen::kBusName,
                           unity::screen::kPath,
                           unity::screen::kInterfaceName,
                           "removeDisplayOnRequest",
                           g_variant_new("(i)", cookie_),
                          nullptr,
                           G_DBUS_CALL_FLAGS_NONE,
                           -1, // Make sure we wait for the service being started up
                           nullptr,
                           [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto thiz = static_cast<ac::SharedKeepAlive<UnityDisplayLock>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        auto result = g_dbus_connection_call_finish(thiz->connection_.get(), res, &error);
        if (!result) {
            ERROR("Failed to acquire display lock: %s", error->message);
            g_error_free(error);
            return;
        }

        thiz->cookie_ = 0;
        thiz->ref_count_ = 0;

        DEBUG("Successfully unlocked display");

    }, new ac::SharedKeepAlive<UnityDisplayLock>{shared_from_this()});
}

void UnityDisplayLock::Release(DisplayState state) {
    if (ref_count_ > 1) {
        ref_count_--;
        return;
    }

    ReleaseInternal();
}

} // namespace ubuntu
} // namespace ac
