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

#ifndef SCOPED_GOBJECT_H_
#define SCOPED_GOBJECT_H_

#include <memory>

#include <glib-object.h>

namespace mcs {
// A GObjectDeleter considers T to be a GObject and
// provides an operator() that decrements the reference
// count of an instance of T.
template<typename T>
struct GObjectDeleter {
    void operator()(T *object) const {
        if (object)
            g_object_unref(object);
    }
};
// A ScopedGObject instance handles raw GObject instances
// and automatically cleans them up on destruction.
template<typename T>
using ScopedGObject = std::unique_ptr<T, GObjectDeleter<T>>;
}

#endif // SCOPED_GOBJECT_H_
