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

#ifndef DBUSHELPERS_H_
#define DBUSHELPERS_H_

#include <functional>

#include <glib.h>
#include <gio/gio.h>

#include "networkmanager.h"
#include "scoped_gobject.h"

namespace mcs {

struct DBusHelpers {
    static gchar** GenerateCapabilities(const std::vector<NetworkManager::Capability> &capabilities);
    static gchar** GenerateDeviceCapabilities(const std::vector<NetworkDeviceRole> &roles);
    static void ParseDictionary(GVariant *properties, std::function<void(std::string, GVariant*)> callback, const std::string &key_filter = "");
    static void ParseArray(GVariant *array, std::function<void(GVariant*)> callback);
};

} // namespace mcs

#endif
