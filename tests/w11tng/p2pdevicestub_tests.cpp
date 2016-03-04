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

using namespace ::testing;

namespace {
class P2PDeviceStubFixture : public Test,
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
    MOCK_METHOD0(OnPeerConnectFailed, void());
    MOCK_METHOD2(OnGroupOwnerNegotiationSuccess, void(const std::string&, const w11tng::P2PDeviceStub::GroupOwnerNegotiationResult&));
    MOCK_METHOD2(OnGroupOwnerNegotiationFailure, void(const std::string&, const w11tng::P2PDeviceStub::GroupOwnerNegotiationResult &));
    MOCK_METHOD3(OnGroupStarted, void(const std::string&, const std::string&, const std::string&));
    MOCK_METHOD2(OnGroupFinished, void(const std::string&, const std::string&));
    MOCK_METHOD2(OnGroupRequest, void(const std::string&, int));
    MOCK_METHOD0(OnP2PDeviceChanged, void());
    MOCK_METHOD0(OnP2PDeviceReady, void());
};

class MockP2PDeviceSkeletonDelegate : public w11tng::testing::P2PDeviceSkeleton::Delegate {
public:
    MOCK_METHOD0(OnFind, void());
    MOCK_METHOD0(OnStopFind, void());
};
}

TEST(P2PDeviceStub, CorrectPropertyNames) {
    EXPECT_EQ("peer_object", w11tng::P2PDeviceStub::PropertyToString(w11tng::P2PDeviceStub::Property::kPeerObject));
    EXPECT_EQ("status", w11tng::P2PDeviceStub::PropertyToString(w11tng::P2PDeviceStub::Property::kStatus));
    EXPECT_EQ("frequency", w11tng::P2PDeviceStub::PropertyToString(w11tng::P2PDeviceStub::Property::kFrequency));
    EXPECT_EQ("frequency_list", w11tng::P2PDeviceStub::PropertyToString(w11tng::P2PDeviceStub::Property::kFrequencyList));
    EXPECT_EQ("wps_method", w11tng::P2PDeviceStub::PropertyToString(w11tng::P2PDeviceStub::Property::kWpsMethod));
}

TEST(P2PDeviceStub, CorrectWpsMethodNames) {
    // Fallback is always PBC
    EXPECT_EQ(w11tng::P2PDeviceStub::WpsMethod::kPbc, w11tng::P2PDeviceStub::WpsMethodFromString("11"));
    EXPECT_EQ(w11tng::P2PDeviceStub::WpsMethod::kPbc, w11tng::P2PDeviceStub::WpsMethodFromString("pbc"));
    EXPECT_EQ(w11tng::P2PDeviceStub::WpsMethod::kPin, w11tng::P2PDeviceStub::WpsMethodFromString("pin"));

    EXPECT_EQ("pbc", w11tng::P2PDeviceStub::WpsMethodToString(w11tng::P2PDeviceStub::WpsMethod::kPbc));
    EXPECT_EQ("pin", w11tng::P2PDeviceStub::WpsMethodToString(w11tng::P2PDeviceStub::WpsMethod::kPin));
}

TEST_F(P2PDeviceStubFixture, ConstructionAndSetup) {
    auto delegate = std::make_shared<MockP2PDeviceStubDelegate>();

    EXPECT_CALL(*delegate, OnP2PDeviceChanged()).Times(1);
    EXPECT_CALL(*delegate, OnP2PDeviceReady()).Times(1);

    auto skeleton = w11tng::testing::P2PDeviceSkeleton::Create("/device_1");

    auto stub = w11tng::P2PDeviceStub::Create("/device_1", delegate);
    EXPECT_TRUE(!!stub);
    EXPECT_FALSE(stub->Connected());
    EXPECT_FALSE(stub->Scanning());

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_TRUE(stub->Connected());
    EXPECT_FALSE(stub->Scanning());
}

TEST_F(P2PDeviceStubFixture, DeviceFound) {
    auto delegate = std::make_shared<MockP2PDeviceStubDelegate>();

    EXPECT_CALL(*delegate, OnP2PDeviceChanged()).Times(1);
    EXPECT_CALL(*delegate, OnP2PDeviceReady()).Times(1);
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

    EXPECT_CALL(*delegate, OnP2PDeviceChanged()).Times(1);
    EXPECT_CALL(*delegate, OnP2PDeviceReady()).Times(1);
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

TEST_F(P2PDeviceStubFixture, AllOtherSignalsSuccessfullySent) {
    auto delegate = std::make_shared<MockP2PDeviceStubDelegate>();

    w11tng::P2PDeviceStub::GroupOwnerNegotiationResult go_negotiation_success_result;
    ::memset(&go_negotiation_success_result, 0, sizeof(go_negotiation_success_result));

    EXPECT_CALL(*delegate, OnP2PDeviceChanged()).Times(AtLeast(1));
    EXPECT_CALL(*delegate, OnP2PDeviceReady()).Times(1);
    EXPECT_CALL(*delegate, OnGroupOwnerNegotiationSuccess(std::string("/peer_1"), _))
            .Times(1)
            .WillRepeatedly(SaveArg<1>(&go_negotiation_success_result));
    EXPECT_CALL(*delegate, OnGroupOwnerNegotiationFailure(std::string("/peer_1"), _))
            .Times(1);
    EXPECT_CALL(*delegate, OnGroupStarted(std::string("/peer_1"), std::string("/interface_1"), std::string("GO")))
            .Times(1);
    EXPECT_CALL(*delegate, OnGroupFinished(std::string("/peer_1"), std::string("/interface_1")))
            .Times(1);
    EXPECT_CALL(*delegate, OnGroupRequest(std::string("/peer_1"), 1337))
            .Times(1);

    auto skeleton = w11tng::testing::P2PDeviceSkeleton::Create("/device_1");

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    auto stub = w11tng::P2PDeviceStub::Create("/device_1", delegate);
    EXPECT_TRUE(!!stub);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    const auto freqs = w11tng::P2PDeviceStub::FrequencyList{1,2,3};

    skeleton->EmitGroupOwnerNegotiationSuccess("/peer_1", w11tng::P2PDeviceStub::Status::kSuccess,
                                               1337, freqs, w11tng::P2PDeviceStub::WpsMethod::kPin);
    skeleton->EmitGroupOwnerNegotiationFailure("/peer_1");
    skeleton->EmitGroupStarted("/peer_1", "/interface_1", "GO");
    skeleton->EmitGroupFinished("/peer_1", "/interface_1");
    skeleton->EmitGroupRequest("/peer_1", 1337);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_EQ(w11tng::P2PDeviceStub::Status::kSuccess, go_negotiation_success_result.status);
    EXPECT_EQ(1337, go_negotiation_success_result.oper_freq);
    EXPECT_EQ(freqs, go_negotiation_success_result.frequencies);
    EXPECT_EQ(w11tng::P2PDeviceStub::WpsMethod::kPin, go_negotiation_success_result.wps_method);
}

TEST_F(P2PDeviceStubFixture, FindAndTimeoutHandling) {
    auto stub_delegate = std::make_shared<MockP2PDeviceStubDelegate>();
    auto skeleton_delegate = std::make_shared<MockP2PDeviceSkeletonDelegate>();

    EXPECT_CALL(*stub_delegate, OnP2PDeviceReady()).Times(1);
    EXPECT_CALL(*stub_delegate, OnP2PDeviceChanged()).Times(3);

    EXPECT_CALL(*skeleton_delegate, OnFind()).Times(1);
    EXPECT_CALL(*skeleton_delegate, OnStopFind()).Times(1);

    auto skeleton = w11tng::testing::P2PDeviceSkeleton::Create("/device_1");

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    skeleton->SetDelegate(skeleton_delegate);

    auto stub = w11tng::P2PDeviceStub::Create("/device_1", stub_delegate);
    EXPECT_TRUE(!!stub);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_TRUE(stub->Connected());

    stub->Find(std::chrono::seconds{1});

    // If we're waiting a bit longer here we will get the stop find
    // call the stub issues after the timeout.
    mcs::testing::RunMainLoop(std::chrono::seconds{2});
}
