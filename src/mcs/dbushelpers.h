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

#include <glib.h>

#include "networkdevice.h"

namespace mcs {

struct DBusHelpers {
    static gchar** GenerateCapabilities(const std::vector<NetworkDeviceRole> roles);
};

} // namespace mcs

#endif
