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

#include <mcs/miracastcontrollerskeleton.h>

namespace {
struct MockMiracastController : public mcs::MiracastController {
    MOCK_METHOD1(SetDelegate, void(const std::weak_ptr<mcs::MiracastController::Delegate> &));
    MOCK_METHOD0(ResetDelegate, void());

    MOCK_METHOD2(Connect, void(const mcs::NetworkDevice::Ptr &, mcs::ResultCallback));
    MOCK_METHOD2(Disconnect, void(const mcs::NetworkDevice::Ptr &, mcs::ResultCallback));
    MOCK_METHOD1(DisconnectAll, void(mcs::ResultCallback));

    MOCK_METHOD1(Scan, mcs::Error(const std::chrono::seconds &));

    MOCK_CONST_METHOD0(State, mcs::NetworkDeviceState());
    MOCK_CONST_METHOD0(Capabilities, std::vector<mcs::NetworkManager::Capability>());
    MOCK_CONST_METHOD0(Scanning, bool());
    MOCK_CONST_METHOD0(Enabled, bool());

    MOCK_METHOD1(SetEnabled, mcs::Error(bool));
};
}

TEST(MiracastControllerSkeleton, ThrowsForNullptrOnConstruction) {
    EXPECT_THROW(mcs::MiracastControllerSkeleton::create(mcs::MiracastController::Ptr{}), std::logic_error);
}

TEST(MiracastControllerSkeleton, ForwardsCallsToImpl) {
    using namespace testing;

    auto impl = std::make_shared<MockMiracastController>();

    // Times(AtLeast(1)) as MiracastControllerSkeleton::create(...) already calls it.
    // In addition, we have to account for the case where we encounter issues during
    // construction of mcs::MiracastControllerSkeleton (such that a WPA supplicant connection
    // is never set up.
    EXPECT_CALL(*impl, SetDelegate(_)).Times(AtLeast(1));
    EXPECT_CALL(*impl, ResetDelegate()).Times(1);
    EXPECT_CALL(*impl, Connect(_,_)).Times(1);
    EXPECT_CALL(*impl, Disconnect(_,_)).Times(1);
    EXPECT_CALL(*impl, DisconnectAll(_)).Times(1);
    EXPECT_CALL(*impl, Scan(_)).Times(1).WillRepeatedly(Return(mcs::Error::kNone));
    EXPECT_CALL(*impl, State()).Times(1).WillRepeatedly(Return(mcs::NetworkDeviceState::kConnected));
    EXPECT_CALL(*impl, Capabilities()).Times(1).WillRepeatedly(Return(std::vector<mcs::NetworkManager::Capability>{mcs::NetworkManager::Capability::kSource}));
    EXPECT_CALL(*impl, Scanning()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*impl, Enabled()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*impl, SetEnabled(false)).Times(1).WillRepeatedly(Return(mcs::Error::kNone));

    auto fmc = mcs::MiracastControllerSkeleton::create(impl);
    fmc->SetDelegate(std::shared_ptr<mcs::MiracastController::Delegate>{});
    fmc->ResetDelegate();
    fmc->Connect(mcs::NetworkDevice::Ptr{}, mcs::ResultCallback{});
    fmc->Disconnect(mcs::NetworkDevice::Ptr{}, mcs::ResultCallback{});
    fmc->DisconnectAll(mcs::ResultCallback{});
    fmc->Scan(std::chrono::seconds{10});
    fmc->State();
    fmc->Capabilities();
    fmc->Scanning();
    fmc->Enabled();
    fmc->SetEnabled(false);

    Mock::AllowLeak(impl.get());
}
