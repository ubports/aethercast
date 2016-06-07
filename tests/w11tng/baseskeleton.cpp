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

#include "baseskeleton.h"

namespace w11tng {
namespace testing {

template <typename T>
BaseSkeleton<T>::BaseSkeleton(T *instance, const std::string &object_path) {
    GError *error = nullptr;
    bus_connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!bus_connection_) {
        AC_ERROR("Failed to connect with system bus: %s", error->message);
        g_error_free(error);
        return;
    }

    skeleton_.reset(instance);

    if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skeleton_.get()),
                                     bus_connection_.get(), object_path.c_str(), &error)) {
        AC_ERROR("Failed to export interface on path %s: %s", object_path, error->message);
        g_error_free(error);
    }
}

template <typename T>
BaseSkeleton<T>::~BaseSkeleton() {
}

template <typename T>
std::string BaseSkeleton<T>::ObjectPath() const {
    return std::string(g_dbus_interface_skeleton_get_object_path(G_DBUS_INTERFACE_SKELETON(skeleton_.get())));
}

template class BaseSkeleton<WpaSupplicantInterfaceP2PDevice>;
template class BaseSkeleton<WpaSupplicantPeer>;
template class BaseSkeleton<WpaSupplicantInterface>;

} // namespace testing
} // namespace w11tng


