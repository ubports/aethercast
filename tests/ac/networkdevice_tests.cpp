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

#include "mock_network_device.h"

using namespace testing;

TEST(NetworkDevice, IsConnectingReturnsTrueForConfiguration) {
    testing::MockNetworkDevice mnd;

    EXPECT_CALL(mnd, State()).Times(AtLeast(1)).WillRepeatedly(Return(ac::NetworkDeviceState::kConfiguration));
    EXPECT_TRUE(mnd.IsConnecting());
}

TEST(NetworkDevice, IsConnectingReturnsTrueForAssociation) {
    testing::MockNetworkDevice mnd;

    EXPECT_CALL(mnd, State()).Times(AtLeast(1)).WillRepeatedly(Return(ac::NetworkDeviceState::kAssociation));
    EXPECT_TRUE(mnd.IsConnecting());
}
