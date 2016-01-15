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

#include <core/posix/fork.h>

#include <w11tng/dhcplistenerskeleton.h>
#include <w11tng/dhcplistenerstub.h>

#include <common/glibhelpers.h>

namespace {
class MockDhcpListenerSkeletonDelegate : public w11tng::DhcpListenerSkeleton::Delegate {
public:
    MOCK_METHOD0(OnNewConnection, void());
    MOCK_METHOD0(OnConnectionClosed, void());
    MOCK_METHOD1(OnEvent, void(const std::map<std::string, std::string> &));
};
}

TEST(DhcpListener, ConnectionEstablishment) {
    using namespace ::testing;

    auto path = mcs::Utils::Sprintf("%s/ac-dhcplistener-test-%s",
                                    boost::filesystem::temp_directory_path().string(),
                                    boost::filesystem::unique_path().string());

    auto delegate = std::make_shared<MockDhcpListenerSkeletonDelegate>();

    EXPECT_CALL(*delegate, OnNewConnection()).Times(1);
    EXPECT_CALL(*delegate, OnConnectionClosed()).Times(1);
    EXPECT_CALL(*delegate, OnEvent(std::map<std::string,std::string>{std::pair<std::string,std::string>("foo","bar")})).Times(1);

    auto skeleton = w11tng::DhcpListenerSkeleton::Create(path, delegate);

    auto expected_address = mcs::Utils::Sprintf("unix:path=%s", path);

    EXPECT_EQ(skeleton->Address(), expected_address);

    auto stub_process = core::posix::fork([&]() {
        auto result = 0;

        auto stub = w11tng::DhcpListenerStub::Create(expected_address);

        auto props = std::map<std::string,std::string>{};
        props["foo"] = "bar";

        stub->EmitEvent(props);

        return static_cast<core::posix::exit::Status>(result);
    }, core::posix::StandardStream::empty);


    mcs::testing::RunMainLoop(std::chrono::seconds{1});
}
