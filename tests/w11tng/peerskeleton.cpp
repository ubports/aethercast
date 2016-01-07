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

#include "peerskeleton.h"

namespace w11tng {
namespace testing {

PeerSkeleton::PeerSkeleton(const std::string &object_path) {

    GError *error = nullptr;
    bus_connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!bus_connection_) {
        g_error_free(error);
        return;
    }


    skeleton_.reset(wpa_supplicant_peer_skeleton_new());

    SetName("");
    SetAddress(std::vector<uint8_t>{});

    g_bus_own_name(G_BUS_TYPE_SYSTEM, "fi.w1.wpa_supplicant1", G_BUS_NAME_OWNER_FLAGS_NONE,
                   nullptr, nullptr, nullptr, nullptr, nullptr);

    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skeleton_.get()),
                                     bus_connection_.get(), object_path.c_str(), nullptr);
}

void PeerSkeleton::SetAddress(const std::vector<uint8_t> &address) {
    auto value = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, address.size() == 0 ? nullptr : &address[0], address.size(), 1);
    wpa_supplicant_peer_set_device_address(skeleton_.get(), value);
}

void PeerSkeleton::SetName(const std::string &name) {
    wpa_supplicant_peer_set_device_name(skeleton_.get(), name.c_str());
}


} // namespace testing
} // namespace w11tng
