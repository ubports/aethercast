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

#ifndef AC_TESTING_VIRTUAL_ETHERNET_H_
#define AC_TESTING_VIRTUAL_ETHERNET_H_

#include <string>

namespace ac {
namespace testing {

class VirtualNetwork {
public:
    VirtualNetwork();
    ~VirtualNetwork();

    std::string Endpoint1() const;
    std::string Endpoint2() const;

private:
    std::string endpoint1_;
    std::string endpoint2_;
};

} // namespace testing
} // namespace ac

#endif
