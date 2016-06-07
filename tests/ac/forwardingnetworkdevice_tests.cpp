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

#include <gmock/gmock.h>

#include <ac/forwardingnetworkdevice.h>

#include "mock_network_device.h"

TEST(ForwardingNetworkDevice, ThrowsForNullptrOnConstruction) {
    EXPECT_THROW(ac::ForwardingNetworkDevice(ac::NetworkDevice::Ptr{}), std::logic_error);
}

TEST(ForwardingNetworkDevice, ForwardsCallsToImpl) {
    using namespace testing;

    const ac::MacAddress address{"lalelu"};
    const ac::IpV4Address ipV4Address = ac::IpV4Address::from_string("127.0.0.1");
    const std::string name{"MeMyselfAndI"};
    const ac::NetworkDeviceState state{ac::NetworkDeviceState::kDisconnected};
    const std::vector<ac::NetworkDeviceRole> roles{ac::NetworkDeviceRole::kSource};

    auto impl = std::make_shared<MockNetworkDevice>();

    EXPECT_CALL(*impl, Address()).Times(1).WillRepeatedly(Return(address));
    EXPECT_CALL(*impl, IPv4Address()).Times(1).WillRepeatedly(Return(ipV4Address));
    EXPECT_CALL(*impl, Name()).Times(1).WillRepeatedly(Return(name));
    EXPECT_CALL(*impl, State()).Times(1).WillRepeatedly(Return(state));
    EXPECT_CALL(*impl, SupportedRoles()).Times(1).WillRepeatedly(Return(roles));

    ac::ForwardingNetworkDevice fmc{impl};
    EXPECT_EQ(address, fmc.Address());
    EXPECT_EQ(ipV4Address, fmc.IPv4Address());
    EXPECT_EQ(name, fmc.Name());
    EXPECT_EQ(state, fmc.State());
    EXPECT_EQ(roles, fmc.SupportedRoles());}
