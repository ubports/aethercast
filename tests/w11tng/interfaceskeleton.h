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

#ifndef W11TNG_TESTING_INTERFACE_SKELETON_H_
#define W11TNG_TESTING_INTERFACE_SKELETON_H_

#include "baseskeleton.h"

namespace w11tng {
namespace testing {

class InterfaceSkeleton : public BaseSkeleton<WpaSupplicantInterface> {
public:
    typedef std::shared_ptr<InterfaceSkeleton> Ptr;

    InterfaceSkeleton(const std::string &object_path);
    ~InterfaceSkeleton();

    void SetCapabilities(GVariant *value);
    void SetDriver(const std::string &driver);
    void SetIfname(const std::string &ifname);
};

} // namespace testing
} // namespace w11tng

#endif
