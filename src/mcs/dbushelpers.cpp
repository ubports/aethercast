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

#include "dbushelpers.h"

namespace mcs {

gchar** DBusHelpers::GenerateCapabilities(const std::vector<NetworkDeviceRole> roles) {
    gchar** capabilities = g_new0(gchar*, roles.size() + 1);
    int n = 0;
    for (auto role : roles)
        capabilities[n] = g_strdup(NetworkDevice::RoleToStr(roles[n++]).c_str());
    capabilities[n] = nullptr;
    return capabilities;
}

} // namespace mcs



