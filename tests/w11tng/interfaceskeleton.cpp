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

#include "interfaceskeleton.h"

namespace w11tng {
namespace testing {

InterfaceSkeleton::InterfaceSkeleton(const std::string &object_path) {
    GError *error = nullptr;
    bus_connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!bus_connection_) {
        g_error_free(error);
        return;
    }

    skeleton_.reset(wpa_supplicant_interface_skeleton_new());

    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skeleton_.get()),
                                     bus_connection_.get(), object_path.c_str(), nullptr);
}

InterfaceSkeleton::~InterfaceSkeleton() {
}

void InterfaceSkeleton::SetCapabilities(GVariant *value) {
    wpa_supplicant_interface_set_capabilities(skeleton_.get(), value);
}

void InterfaceSkeleton::SetDriver(const std::string &driver) {
    wpa_supplicant_interface_set_driver(skeleton_.get(), driver.c_str());
}

void InterfaceSkeleton::SetIfname(const std::string &ifname) {
    wpa_supplicant_interface_set_ifname(skeleton_.get(), ifname.c_str());
}

std::string InterfaceSkeleton::ObjectPath() const {
    return std::string(g_dbus_interface_skeleton_get_object_path(G_DBUS_INTERFACE_SKELETON(skeleton_.get())));
}

} // namespace testing
} // namespace w11tng
