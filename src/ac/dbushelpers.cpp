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

namespace ac {

gchar** DBusHelpers::GenerateDeviceCapabilities(const std::vector<NetworkDeviceRole> &roles) {
    gchar **capabilities = g_new0(gchar*, roles.size() + 1);
    int n = 0;
    for (auto role : roles)
        capabilities[n] = g_strdup(NetworkDevice::RoleToStr(role).c_str());
    capabilities[n] = nullptr;
    return capabilities;
}

gchar** DBusHelpers::GenerateCapabilities(const std::vector<NetworkManager::Capability> &capabilities) {
    gchar** out_capabilities = g_new0(gchar*, capabilities.size() + 1);
    int n = 0;
    for (auto capability : capabilities) {
        auto value = NetworkManager::CapabilityToStr(capability);
        out_capabilities[n++] = g_strdup(value.c_str());
    }
    out_capabilities[n] = nullptr;
    return out_capabilities;
}

void DBusHelpers::ParseDictionary(GVariant *properties, std::function<void(std::string, GVariant*)> callback, const std::string &key_filter) {
    if (!callback || !properties)
        return;

    for (int n = 0; n < g_variant_n_children(properties); n++) {
        auto property = g_variant_get_child_value(properties, n);
        auto key_v = g_variant_get_child_value(property, 0);
        auto value_v = g_variant_get_child_value(property, 1);

        std::string key = g_variant_get_string(key_v, nullptr);

        if (key_filter.length() > 0) {
            if (key_filter != key)
                continue;

            callback(key, value_v);
        }
        else
            callback(key, value_v);
    }
}

void DBusHelpers::ParseArray(GVariant *array, std::function<void(GVariant*)> callback) {
    if (!callback || !array)
        return;

    for (int n = 0; n < g_variant_n_children(array); n++) {
        auto element = g_variant_get_child_value(array, n);
        callback(element);
    }
}

} // namespace ac



