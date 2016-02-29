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

#include <core/posix/fork.h>

#include <common/glibhelpers.h>
#include <common/dbusfixture.h>
#include <common/dbusnameowner.h>

#include <mcs/keep_alive.h>
#include <w11tng/interfacestub.h>

#include "interfaceskeleton.h"

namespace {
class InterfaceStubFixture : public ::testing::Test,
                        public mcs::testing::DBusFixture,
                        public mcs::testing::DBusNameOwner {
public:
    InterfaceStubFixture() :
        mcs::testing::DBusNameOwner("fi.w1.wpa_supplicant1") {
    }
};

class MockInterfaceStubDelegate : public w11tng::InterfaceStub::Delegate {
public:
    MOCK_METHOD1(OnInterfaceReady, void(const std::string&));
    MOCK_METHOD1(OnInterfaceDriverCommandResult, void(const std::string&));
};
}

TEST_F(InterfaceStubFixture, ConstructionAndProperties) {

    auto skeleton = std::make_shared<w11tng::testing::InterfaceSkeleton>("/interface_1");

    auto delegate = std::make_shared<MockInterfaceStubDelegate>();

    EXPECT_CALL(*delegate, OnInterfaceReady(::testing::_)).Times(1);

    auto stub = w11tng::InterfaceStub::Create("/interface_1");
    EXPECT_TRUE(!!stub);
    stub->SetDelegate(delegate);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_EQ(stub->Capabilities().size(), 0);
    EXPECT_EQ(stub->Driver(), "");
    EXPECT_EQ(stub->Ifname(), "");

    skeleton->SetDriver("nl80211");
    skeleton->SetIfname("wlan0");

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_EQ(stub->Driver(), "nl80211");
    EXPECT_EQ(stub->Ifname(), "wlan0");
}
