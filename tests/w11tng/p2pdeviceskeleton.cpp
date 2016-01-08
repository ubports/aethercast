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

#include "p2pdeviceskeleton.h"

namespace w11tng {
namespace testing {

P2PDeviceSkeleton::P2PDeviceSkeleton(const std::string &object_path) :
    BaseSkeleton(wpa_supplicant_interface_p2_pdevice_skeleton_new(), object_path) {
}

P2PDeviceSkeleton::~P2PDeviceSkeleton() {
}

} // namespace testing
} // namespace w11tng
