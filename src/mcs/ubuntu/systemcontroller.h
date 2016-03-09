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

#ifndef MCS_UBUNTU_SYSTEMCONTROLLER_H_
#define MCS_UBUNTU_SYSTEMCONTROLLER_H_

#include "mcs/systemcontroller.h"

namespace mcs {
namespace ubuntu {

class SystemController : public mcs::SystemController {
public:
    SystemController();
    ~SystemController();

    Lock<DisplayState>::Ptr DisplayStateLock();

private:
    Lock<mcs::DisplayState>::Ptr display_lock_;
};

} // namespace ubuntu
} // namespace mcs

#endif
