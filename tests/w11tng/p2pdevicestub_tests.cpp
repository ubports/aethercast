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

#include <cstdint>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <common/glibhelpers.h>
#include <common/dbusfixture.h>
#include <common/dbusnameowner.h>

#include <w11tng/p2pdevicestub.h>

#include "p2pdeviceskeleton.h"

namespace {
class P2PDeviceStubFixture : public ::testing::Test,
                        public mcs::testing::DBusFixture,
                        public mcs::testing::DBusNameOwner {
public:
    P2PDeviceStubFixture() :
        mcs::testing::DBusNameOwner("fi.w1.wpa_supplicant1") {
    }
};

class MockP2PDeviceStubDelegate : public w11tng::P2PDeviceStub::Delegate {
public:
    MOCK_METHOD1(OnDeviceFound, void(const std::string&));
    MOCK_METHOD1(OnDeviceLost, void(const std::string&));
    MOCK_METHOD1(OnGroupOwnerNegotiationSuccess, void(const std::string&));
    MOCK_METHOD1(OnGroupOwnerNegotiationFailure, void(const std::string&));
    MOCK_METHOD3(OnGroupStarted, void(const std::string&, const std::string&, const std::string&));
    MOCK_METHOD2(OnGroupFinished, void(const std::string&, const std::string&));
    MOCK_METHOD1(OnGroupRequest, void(const std::string&));
    MOCK_METHOD0(OnChanged, void());
};

class MockP2PDeviceSkeletonDelegate : public w11tng::testing::P2PDeviceSkeleton::Delegate {
public:
    MOCK_METHOD0(OnFind, void());
    MOCK_METHOD0(OnStopFind, void());
};
}

TEST_F(P2PDeviceStubFixture, ConstructionAndSetup) {
    auto delegate = std::make_shared<MockP2PDeviceStubDelegate>();

    // Called to mark the stub as ready
    EXPECT_CALL(*delegate, OnChanged()).Times(1);

    auto skeleton = w11tng::testing::P2PDeviceSkeleton::Create("/device_1");

    auto stub = w11tng::P2PDeviceStub::Create("/device_1", delegate);
    EXPECT_TRUE(!!stub);
    EXPECT_FALSE(stub->Connected());
    EXPECT_FALSE(stub->Scanning());

    unsigned int ready_called = 0;
    stub->Ready().connect([&]() { ready_called++; });

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_EQ(ready_called, 1);
    EXPECT_TRUE(stub->Connected());
    EXPECT_FALSE(stub->Scanning());
}

TEST_F(P2PDeviceStubFixture, DeviceFound) {
    auto delegate = std::make_shared<MockP2PDeviceStubDelegate>();

    // Called to mark the stub as ready
    EXPECT_CALL(*delegate, OnChanged()).Times(1);
    EXPECT_CALL(*delegate, OnDeviceFound(std::string("/peer_1"))).Times(2);

    auto skeleton = w11tng::testing::P2PDeviceSkeleton::Create("/device_1");

    auto stub = w11tng::P2PDeviceStub::Create("/device_1", delegate);
    EXPECT_TRUE(!!stub);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_TRUE(stub->Connected());

    // Emit the signal twos times for the same peer to make sure
    // both are going through even if they have the same payload.
    skeleton->EmitDeviceFound("/peer_1");
    skeleton->EmitDeviceFound("/peer_1");

    mcs::testing::RunMainLoop(std::chrono::seconds{1});
}


TEST_F(P2PDeviceStubFixture, DeviceLost) {
    auto delegate = std::make_shared<MockP2PDeviceStubDelegate>();

    // Called to mark the stub as ready
    EXPECT_CALL(*delegate, OnChanged()).Times(1);
    EXPECT_CALL(*delegate, OnDeviceLost(std::string("/peer_1"))).Times(2);

    auto skeleton = w11tng::testing::P2PDeviceSkeleton::Create("/device_1");

    auto stub = w11tng::P2PDeviceStub::Create("/device_1", delegate);
    EXPECT_TRUE(!!stub);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_TRUE(stub->Connected());

    // Emit the signal twos times for the same peer to make sure
    // both are going through even if they have the same payload.
    skeleton->EmitDeviceLost("/peer_1");
    skeleton->EmitDeviceLost("/peer_1");

    mcs::testing::RunMainLoop(std::chrono::seconds{1});
}

/*
TEST_F(P2PDeviceStubFixture, FindAndTimeoutHandling) {
    auto stub_delegate = std::make_shared<MockP2PDeviceStubDelegate>();
    auto skeleton_delegate = std::make_shared<MockP2PDeviceSkeletonDelegate>();

    // Called to mark the stub as ready
    EXPECT_CALL(*stub_delegate, OnChanged()).Times(1);

    EXPECT_CALL(*skeleton_delegate, OnFind()).Times(1);
    //EXPECT_CALL(*skeleton_delegate, OnStopFind()).Times(1);

    auto skeleton = w11tng::testing::P2PDeviceSkeleton::Create("/device_1");

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    skeleton->SetDelegate(skeleton_delegate);

    auto stub = w11tng::P2PDeviceStub::Create("/device_1", stub_delegate);
    EXPECT_TRUE(!!stub);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_TRUE(stub->Connected());

    stub->Find(std::chrono::seconds{1});

    mcs::testing::RunMainLoop(std::chrono::seconds{1});
}
*/
