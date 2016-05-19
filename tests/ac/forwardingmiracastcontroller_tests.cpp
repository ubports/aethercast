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

#include <ac/forwardingmiracastcontroller.h>

namespace {
struct MockMiracastController : public ac::MiracastController {
    MOCK_METHOD1(SetDelegate, void(const std::weak_ptr<ac::MiracastController::Delegate> &));
    MOCK_METHOD0(ResetDelegate, void());

    MOCK_METHOD2(Connect, void(const ac::NetworkDevice::Ptr &, ac::ResultCallback));
    MOCK_METHOD2(Disconnect, void(const ac::NetworkDevice::Ptr &, ac::ResultCallback));
    MOCK_METHOD1(DisconnectAll, void(ac::ResultCallback));

    MOCK_METHOD1(Scan, ac::Error(const std::chrono::seconds &));

    MOCK_CONST_METHOD0(State, ac::NetworkDeviceState());
    MOCK_CONST_METHOD0(Capabilities, std::vector<ac::NetworkManager::Capability>());
    MOCK_CONST_METHOD0(Scanning, bool());
    MOCK_CONST_METHOD0(Enabled, bool());

    MOCK_METHOD1(SetEnabled, ac::Error(bool));
};
}

TEST(ForwardingMiracastController, ThrowsForNullptrOnConstruction) {
    EXPECT_THROW(ac::ForwardingMiracastController(ac::MiracastController::Ptr{}), std::logic_error);
}

TEST(ForwardingMiracastController, ForwardsCallsToImpl) {
    using namespace testing;

    auto impl = std::make_shared<MockMiracastController>();

    EXPECT_CALL(*impl, SetDelegate(_)).Times(1);
    EXPECT_CALL(*impl, ResetDelegate()).Times(1);
    EXPECT_CALL(*impl, Connect(_,_)).Times(1);
    EXPECT_CALL(*impl, Disconnect(_,_)).Times(1);
    EXPECT_CALL(*impl, DisconnectAll(_)).Times(1);
    EXPECT_CALL(*impl, Scan(_)).Times(1).WillRepeatedly(Return(ac::Error::kNone));
    EXPECT_CALL(*impl, State()).Times(1).WillRepeatedly(Return(ac::NetworkDeviceState::kConnected));
    EXPECT_CALL(*impl, Capabilities()).Times(1).WillRepeatedly(Return(std::vector<ac::NetworkManager::Capability>{ac::NetworkManager::Capability::kSource}));
    EXPECT_CALL(*impl, Scanning()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*impl, Enabled()).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*impl, SetEnabled(false)).Times(1).WillRepeatedly(Return(ac::Error::kNone));

    ac::ForwardingMiracastController fmc{impl};
    fmc.SetDelegate(std::shared_ptr<ac::MiracastController::Delegate>{});
    fmc.ResetDelegate();
    fmc.Connect(ac::NetworkDevice::Ptr{}, ac::ResultCallback{});
    fmc.Disconnect(ac::NetworkDevice::Ptr{}, ac::ResultCallback{});
    fmc.DisconnectAll(ac::ResultCallback{});
    fmc.Scan(std::chrono::seconds{10});
    fmc.State();
    fmc.Capabilities();
    fmc.Scanning();
    fmc.Enabled();
    fmc.SetEnabled(false);
}
