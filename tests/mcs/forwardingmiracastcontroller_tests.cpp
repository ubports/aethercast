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

#include <mcs/forwardingmiracastcontroller.h>

namespace {
struct MockMiracastController : public mcs::MiracastController {
    MOCK_METHOD1(SetDelegate, void(const std::weak_ptr<mcs::MiracastController::Delegate> &));
    MOCK_METHOD0(ResetDelegate, void());

    MOCK_METHOD2(Connect, void(const mcs::NetworkDevice::Ptr &, mcs::ResultCallback));
    MOCK_METHOD2(Disconnect, void(const mcs::NetworkDevice::Ptr &, mcs::ResultCallback));

    MOCK_METHOD1(Scan, void(const std::chrono::seconds &));

    MOCK_CONST_METHOD0(State, mcs::NetworkDeviceState());
    MOCK_CONST_METHOD0(SupportedRoles, std::vector<mcs::NetworkDeviceRole>());
    MOCK_CONST_METHOD0(Scanning, bool());
};
}

TEST(ForwardingMiracastController, ThrowsForNullptrOnConstruction) {
    EXPECT_THROW(mcs::ForwardingMiracastController(mcs::MiracastController::Ptr{}), std::logic_error);
}

TEST(ForwardingMiracastController, ForwardsCallsToImpl) {
    using namespace testing;

    auto impl = std::make_shared<MockMiracastController>();

    EXPECT_CALL(*impl, SetDelegate(_)).Times(1);
    EXPECT_CALL(*impl, ResetDelegate()).Times(1);
    EXPECT_CALL(*impl, Connect(_,_)).Times(1);
    EXPECT_CALL(*impl, Disconnect(_,_)).Times(1);
    EXPECT_CALL(*impl, Scan(_)).Times(1);
    EXPECT_CALL(*impl, State()).Times(1).WillRepeatedly(Return(mcs::NetworkDeviceState::kConnected));
    EXPECT_CALL(*impl, SupportedRoles()).Times(1).WillRepeatedly(Return(std::vector<mcs::NetworkDeviceRole>{mcs::NetworkDeviceRole::kSource}));
    EXPECT_CALL(*impl, Scanning()).Times(1).WillRepeatedly(Return(true));

    mcs::ForwardingMiracastController fmc{impl};
    fmc.SetDelegate(std::shared_ptr<mcs::MiracastController::Delegate>{});
    fmc.ResetDelegate();
    fmc.Connect(mcs::NetworkDevice::Ptr{}, mcs::ResultCallback{});
    fmc.Disconnect(mcs::NetworkDevice::Ptr{}, mcs::ResultCallback{});
    fmc.Scan(std::chrono::seconds{10});
    fmc.State();
    fmc.SupportedRoles();
    fmc.Scanning();
}
