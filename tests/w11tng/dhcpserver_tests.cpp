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
    MOCK_METHOD0(OnLeaseAdded, void());
};
class MockDhcpClientDelegate : public w11tng::DhcpClient::Delegate {
public:
    MOCK_METHOD1(OnAddressAssigned, void(const mcs::IpV4Address &));
    MOCK_METHOD0(OnNoLease, void());
};
}

TEST(DhcpServer, Startup) {
    mcs::testing::VirtualNetwork veth;

    auto server_delegate = std::make_shared<MockDhcpServerDelegate>();
    auto client_delegate = std::make_shared<MockDhcpClientDelegate>();

    auto server = w11tng::DhcpServer::Create(server_delegate.get(), veth.Endpoint1());
    auto client = w11tng::DhcpClient::Create(client_delegate.get(), veth.Endpoint2());

    server->Start();
    client->Start();

    mcs::testing::RunMainLoop(std::chrono::seconds{2});
}
