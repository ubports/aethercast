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

#ifndef AC_DBUSERRORS_H_
#define AC_DBUSERRORS_H_

#include "ac/types.h"
#include "ac/glib_wrapper.h"

namespace ac {

#define AETHERCAST_ERROR (aethercast_error_quark())
GQuark aethercast_error_quark(void);

typedef enum {
    AETHERCAST_ERROR_FAILED,
    AETHERCAST_ERROR_ALREADY,
    AETHERCAST_ERROR_PARAM_INVALID,
    AETHERCAST_ERROR_INVALID_STATE,
    AETHERCAST_ERROR_NOT_CONNECTED,
    AETHERCAST_ERROR_NOT_READY,
    AETHERCAST_ERROR_IN_PROGRESS,
    AETHERCAST_N_ERRORS,
} AethercastError;

AethercastError AethercastErrorFromError(const Error &error);

} // namespace ac

#endif
