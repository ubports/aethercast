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

#ifndef MCS_UBUNTU_POWERDDISPLAYLOCK_H_
#define MCS_UBUNTU_POWERDDISPLAYLOCK_H_

#include "mcs/glib_wrapper.h"

#include "mcs/scoped_gobject.h"
#include "mcs/systemcontroller.h"

namespace mcs {
namespace ubuntu {

class UnityDisplayLock : public SystemController::Lock<DisplayState>,
                         public std::enable_shared_from_this<UnityDisplayLock> {
public:
    typedef std::shared_ptr<UnityDisplayLock> Ptr;

    static Ptr Create();

    ~UnityDisplayLock();

    void Acquire(DisplayState state);
    void Release(DisplayState state);

private:
    UnityDisplayLock();

    void ReleaseInternal();

private:
    mcs::ScopedGObject<GDBusConnection> connection_;
    unsigned int ref_count_;
    int cookie_;
};

} // namespace ubuntu
} // namespace mcs

#endif
