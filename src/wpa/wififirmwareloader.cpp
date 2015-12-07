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

#include <chrono>

#include <mcs/logger.h>
#include <mcs/networkutils.h>
#include <mcs/utils.h>

#include "wififirmwareloader.h"

namespace wpa {

WiFiFirmwareLoader::WiFiFirmwareLoader(const std::string &interface_name, Delegate *delegate) :
    interface_name_(interface_name),
    delegate_(delegate),
    reload_timeout_source_(0) {
    // FIXME we need to add some change events to wpa to see when the firmware changes
    // so that we can react on this.
}

WiFiFirmwareLoader::~WiFiFirmwareLoader() {
    if (reload_timeout_source_ > 0)
        g_source_remove(reload_timeout_source_);
}

bool WiFiFirmwareLoader::IsNeeded() {
    auto path = mcs::Utils::Sprintf("/sys/class/net/%s/uevent", interface_name_.c_str());
    return !g_file_test(path.c_str(), (GFileTest) (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR));
}

bool WiFiFirmwareLoader::TryLoad() {
    if (!IsNeeded())
        return false;

    auto conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);

    GVariant *params = g_variant_new("(os)", "/", "p2p");
    g_dbus_connection_call(conn, "fi.w1.wpa_supplicant1", "/fi/w1/wpa_supplicant1",
                           "fi.w1.wpa_supplicant1", "SetInterfaceFirmware", params,
                           nullptr, (GDBusCallFlags) G_DBUS_CALL_FLAGS_NONE, -1,
                           nullptr, (GAsyncReadyCallback) &WiFiFirmwareLoader::OnInterfaceFirmwareSet, this);

    return true;
}

gboolean WiFiFirmwareLoader::OnRetryLoad(gpointer user_data) {
    auto inst = static_cast<WiFiFirmwareLoader*>(user_data);

    if (inst->IsNeeded()) {
        inst->TryLoad();
        return FALSE;
    }

    if (inst->delegate_)
        inst->delegate_->OnFirmwareLoaded();

    return FALSE;
}

void WiFiFirmwareLoader::OnInterfaceFirmwareSet(GDBusConnection *conn, GAsyncResult *res, gpointer user_data) {
    auto inst = static_cast<WiFiFirmwareLoader*>(user_data);
    auto timeout = std::chrono::milliseconds(1000);
    GError *error = nullptr;

    GVariant *result = g_dbus_connection_call_finish(conn, res, &error);
    if (!result) {
        MCS_WARNING("Failed to load required WiFi firmware: %s", error->message);
        g_error_free(error);
        timeout = std::chrono::milliseconds(2000);
    }

    inst->reload_timeout_source_ = g_timeout_add(timeout.count(), &WiFiFirmwareLoader::OnRetryLoad, inst);
}


} // namespace wpa
