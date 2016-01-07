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

#include <gio/gio.h>

#include "dbusnameowner.h"

namespace mcs {
namespace testing {

DBusNameOwner::DBusNameOwner(const std::string &name) {
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr));

    // We need to mark the connection as not to terminate ourself when
    // the connection to the bus is lost as we restart the bus for every
    // test we run.
    g_dbus_connection_set_exit_on_close(connection_.get(), FALSE);

    id_ = g_bus_own_name_on_connection(connection_.get(), "fi.w1.wpa_supplicant1", G_BUS_NAME_OWNER_FLAGS_NONE,
                                       nullptr, nullptr, nullptr, nullptr);
}

DBusNameOwner::~DBusNameOwner() {
    g_bus_unown_name(id_);
}

} // namespace testing
} // namespace mcs
