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

#ifndef MCS_GLIBWRAPPER_H_
#define MCS_GLIBWRAPPER_H_

// Ignore all warnings coming from the external GLib headers as
// we don't control them and also don't want to get any warnings
// from them which will only polute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include <glib.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include <glib-object.h>
#pragma GCC diagnostic pop

#endif
