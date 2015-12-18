/*
 * Copyright (C) 2015 Canonical, Ltd.
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

#ifndef MOCK_NETWORK_DEVICE_H_
#define MOCK_NETWORK_DEVICE_H_

#include <gmock/gmock.h>

#include <mcs/networkdevice.h>

namespace testing {
struct MockNetworkDevice : public mcs::NetworkDevice {
    MOCK_CONST_METHOD0(Address, mcs::MacAddress());
    MOCK_CONST_METHOD0(IPv4Address, mcs::IpV4Address());
    MOCK_CONST_METHOD0(Name, std::string());
    MOCK_CONST_METHOD0(State, mcs::NetworkDeviceState());
    MOCK_CONST_METHOD0(SupportedRoles, std::vector<mcs::NetworkDeviceRole>());
};
}

#endif
