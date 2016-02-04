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

#ifndef MCS_TESTING_DBUSNAMEOWNER_H_
#define MCS_TESTING_DBUSNAMEOWNER_H_

#include <glib.h>
#include <gio/gio.h>

#include <string>

#include <mcs/scoped_gobject.h>

namespace mcs {
namespace testing {

class DBusNameOwner {
public:
    DBusNameOwner(const std::string &name);
    ~DBusNameOwner();

private:
    guint id_;
    mcs::ScopedGObject<GDBusConnection> connection_;
};

} // namespace testing
} // namespace mcs

#endif
