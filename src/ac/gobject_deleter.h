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
#ifndef GOBJECT_DELETER_H_
#define GOBJECT_DELETER_H_

#include <glib-object.h>

#include <ac/logger.h>

namespace ac {
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
}

#endif // GOBJECT_DELETER_H_
