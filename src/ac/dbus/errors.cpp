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

#include "ac/dbus/errors.h"

namespace ac {
namespace dbus {

static const GDBusErrorEntry kAethercastErrorEntries[] = {
    { AETHERCAST_ERROR_FAILED, "org.aethercast.Error.Failed" },
    { AETHERCAST_ERROR_ALREADY, "org.aethercast.Error.Already" },
    { AETHERCAST_ERROR_PARAM_INVALID, "org.aethercast.Error.ParamInvalid" },
    { AETHERCAST_ERROR_INVALID_STATE, "org.aethercast.Error.InvalidState" },
    { AETHERCAST_ERROR_NOT_CONNECTED, "org.aethercast.Error.NotConnected" },
    { AETHERCAST_ERROR_NOT_READY, "org.aethercast.Error.NotReady" },
    { AETHERCAST_ERROR_IN_PROGRESS, "org.aethercast.Error.InProgress" },
};

G_STATIC_ASSERT(G_N_ELEMENTS(kAethercastErrorEntries) == AETHERCAST_N_ERRORS);

GQuark aethercast_error_quark () {
    static volatile gsize quark_volatile = 0;
    g_dbus_error_register_error_domain ("aethercast-error-quark",
                                        &quark_volatile,
                                        kAethercastErrorEntries,
                                        G_N_ELEMENTS (kAethercastErrorEntries));
    return (GQuark) quark_volatile;
}

AethercastError AethercastErrorFromError(const Error &error) {
    switch (error) {
    case Error::kFailed:
        return AETHERCAST_ERROR_FAILED;
    case Error::kAlready:
        return AETHERCAST_ERROR_ALREADY;
    case Error::kParamInvalid:
        return AETHERCAST_ERROR_PARAM_INVALID;
    case Error::kInvalidState:
        return AETHERCAST_ERROR_INVALID_STATE;
    case Error::kNotConnected:
        return AETHERCAST_ERROR_NOT_CONNECTED;
    case Error::kNotReady:
        return AETHERCAST_ERROR_NOT_READY;
    default:
        break;
    }

    return AETHERCAST_ERROR_FAILED;
}

} // namespace dbus
} // namespace ac
