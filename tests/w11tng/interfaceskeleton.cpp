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

#include "interfaceskeleton.h"

namespace w11tng {
namespace testing {

InterfaceSkeleton::InterfaceSkeleton(const std::string &object_path) :
    BaseSkeleton(wpa_supplicant_interface_skeleton_new(), object_path) {
}

InterfaceSkeleton::~InterfaceSkeleton() {
}

void InterfaceSkeleton::SetCapabilities(GVariant *value) {
    wpa_supplicant_interface_set_capabilities(skeleton_.get(), value);
}

void InterfaceSkeleton::SetDriver(const std::string &driver) {
    wpa_supplicant_interface_set_driver(skeleton_.get(), driver.c_str());
}

void InterfaceSkeleton::SetIfname(const std::string &ifname) {
    wpa_supplicant_interface_set_ifname(skeleton_.get(), ifname.c_str());
}

} // namespace testing
} // namespace w11tng
