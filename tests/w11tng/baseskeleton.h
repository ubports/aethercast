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

#ifndef W11TNG_TESTING_BASE_SKELETON_H_
#define W11TNG_TESTING_BASE_SKELETON_H_

#include <mcs/glib_wrapper.h>
#include <mcs/scoped_gobject.h>

extern "C" {
// Ignore all warnings coming from the external headers as we don't
// control them and also don't want to get any warnings from them
// which will only pollute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include "wpasupplicantinterface.h"
#pragma GCC diagnostic pop
}

namespace w11tng {
namespace testing {

template <typename T>
class BaseSkeleton {
public:
    BaseSkeleton(T *instance, const std::string &object_path);
    virtual ~BaseSkeleton();

    std::string ObjectPath() const;

protected:
    mcs::ScopedGObject<GDBusConnection> bus_connection_;
    mcs::ScopedGObject<WpaSupplicantObjectProxy> proxy_;
    mcs::ScopedGObject<T> skeleton_;
};

} // namespace testing
} // namespace w11tng

#endif
