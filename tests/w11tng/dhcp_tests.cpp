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

#include <boost/filesystem.hpp>

#include <w11tng/dhcpserver.h>
#include <w11tng/dhcpclient.h>

#include <core/posix/process.h>

#include <common/glibhelpers.h>
#include <common/virtualnetwork.h>

namespace {
class MockDhcpServerDelegate : public w11tng::DhcpServer::Delegate {
public:
    MOCK_METHOD2(OnDhcpAddressAssigned, void(const mcs::IpV4Address &, const mcs::IpV4Address&));
    MOCK_METHOD0(OnDhcpTerminated, void());
};
class MockDhcpClientDelegate : public w11tng::DhcpClient::Delegate {
public:
    MOCK_METHOD2(OnDhcpAddressAssigned, void(const mcs::IpV4Address &, const mcs::IpV4Address&));
    MOCK_METHOD0(OnDhcpTerminated, void());
};
}

TEST(Dhcp, DISABLED_AddressAssignment) {
    mcs::testing::VirtualNetwork veth;

    boost::filesystem::create_directory("/run/aethercast");

    auto server_delegate = std::make_shared<MockDhcpServerDelegate>();
    EXPECT_CALL(*server_delegate, OnDhcpAddressAssigned(mcs::IpV4Address::from_string("192.168.7.1"),
                                                    mcs::IpV4Address::from_string("192.168.7.5")))
            .Times(1);

    auto client_delegate = std::make_shared<MockDhcpClientDelegate>();
    EXPECT_CALL(*client_delegate, OnDhcpAddressAssigned(mcs::IpV4Address::from_string("192.168.7.5"),
                                                    mcs::IpV4Address::from_string("192.168.7.1")))
            .Times(1);

    auto server = w11tng::DhcpServer::Create(server_delegate, veth.Endpoint1());
    auto client = w11tng::DhcpClient::Create(client_delegate, veth.Endpoint2());

    mcs::testing::RunMainLoop(std::chrono::seconds{5});

    EXPECT_EQ(client->LocalAddress().to_string(), "192.168.7.5");
    EXPECT_EQ(client->RemoteAddress().to_string(), "192.168.7.1");
}
