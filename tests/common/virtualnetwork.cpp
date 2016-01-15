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


extern "C" {
#include "3rd_party/lxc-nl/network.h"
}

#include "virtualnetwork.h"

namespace {
static constexpr char* kVethNameTemplate{"vethXXXXXX"};
}

namespace mcs {
namespace testing {

VirtualNetwork::VirtualNetwork() :
    endpoint1_(lxc_mkifname(kVethNameTemplate)),
    endpoint2_(lxc_mkifname(kVethNameTemplate)) {

    lxc_veth_create(endpoint1_.c_str(), endpoint2_.c_str());
    lxc_netdev_up(endpoint1_.c_str());
    lxc_netdev_up(endpoint2_.c_str());
}

VirtualNetwork::~VirtualNetwork() {
    lxc_netdev_delete_by_name(endpoint1_.c_str());
    lxc_netdev_delete_by_name(endpoint2_.c_str());
}

std::string VirtualNetwork::Endpoint1() const {
    return endpoint1_;
}

std::string VirtualNetwork::Endpoint2() const {
    return endpoint2_;
}

} // namespace testing
} // namespace mcs

