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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

#include <common/glibhelpers.h>
#include <common/virtualnetwork.h>

#include <mcs/networkutils.h>

#include <w11tng/netlinklistener.h>

namespace {
class MockNetlinkListenerDelegate : public w11tng::NetlinkListener::Delegate {
public:
    MOCK_METHOD2(OnInterfaceAddressChanged, void(const std::string &, const std::string &));
};
class NetlinkListener : public ::testing::TestWithParam<bool> {
};
}

TEST_P(NetlinkListener, NotifiesIpAddressChanges) {
    auto delegate = std::make_shared<MockNetlinkListenerDelegate>();

    mcs::testing::VirtualNetwork veth;

    auto listener = w11tng::NetlinkListener::Create(delegate);
    if (GetParam())
        listener->SetInterfaceFilter(veth.Endpoint1());

    std::string expected_address = "192.168.7.1";

    // We can't really influence the number of times this is
    // called as the events come in multiple times (for whatever
    // reason) so we require it to happen at least once.
    EXPECT_CALL(*delegate, OnInterfaceAddressChanged(veth.Endpoint1(), expected_address))
            .Times(::testing::AtLeast(1));

    auto interface_index = mcs::NetworkUtils::RetrieveInterfaceIndex(veth.Endpoint1().c_str());
    mcs::NetworkUtils::ModifyInterfaceAddress(RTM_NEWADDR, NLM_F_REPLACE | NLM_F_ACK,
                                              interface_index, AF_INET, expected_address.c_str(),
                                              nullptr, 24, "255.255.255.0");

    mcs::testing::RunMainLoop(std::chrono::seconds{2});
}

INSTANTIATE_TEST_CASE_P(InterfaceAddressChanges, NetlinkListener, ::testing::Values(false, true));
