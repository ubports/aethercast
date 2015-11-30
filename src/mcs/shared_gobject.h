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

#ifndef SHARED_GOBJECT_H_
#define SHARED_GOBJECT_H_

#include <memory>

#include <glib-object.h>

#include "gobject_deleter.h"

namespace mcs {
// A SharedGObject instance handles raw GObject instances
// and automatically cleans them up on destruction.
template<typename T>
using SharedGObject = std::shared_ptr<T>;

template<typename T>
SharedGObject<T> make_shared_gobject(T *gobject) {
    return SharedGObject<T>{gobject, GObjectDeleter<T>{}};
}
}

#endif // SHARED_GOBJECT_H_
