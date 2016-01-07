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

#include <mcs/keep_alive.h>
#include <w11tng/peerstub.h>

#include <common/glibhelpers.h>
#include <common/dbusfixture.h>
#include <common/dbusnameowner.h>

#include "peerskeleton.h"

namespace {
class PeerStubFixture : public ::testing::Test,
                        public mcs::testing::DBusFixture,
                        public mcs::testing::DBusNameOwner {
public:
    PeerStubFixture() :
        mcs::testing::DBusNameOwner("fi.w1.wpa_supplicant1") {
    }
};

class MockPeerDelegate : public w11tng::PeerStub::Delegate {
public:
    MOCK_METHOD0(OnPeerReady, void());
    MOCK_METHOD0(OnPeerChanged, void());
};
}

TEST_F(PeerStubFixture, ConstructionAndProperties) {
    auto delegate = std::make_shared<MockPeerDelegate>();

    EXPECT_CALL(*delegate, OnPeerReady()).Times(1);
    EXPECT_CALL(*delegate, OnPeerChanged()).Times(2);

    auto skeleton = std::make_shared<w11tng::testing::PeerSkeleton>("/peer_1");

    auto stub = w11tng::PeerStub::Create("/peer_1");
    EXPECT_TRUE(!!stub);

    stub->SetDelegate(delegate);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_EQ(stub->Address(), "00:00:00:00:00:00");
    EXPECT_EQ(stub->Name(), std::string(""));

    skeleton->SetAddress(std::vector<uint8_t>{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff });
    skeleton->SetName("Test Peer");

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    EXPECT_EQ(stub->Address(), "aa:bb:cc:dd:ee:ff");
    EXPECT_EQ(stub->Name(), std::string("Test Peer"));
}
