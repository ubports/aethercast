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

#include <glib.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

#include "wpa/wpasupplicantcommandqueue.h"

using ::testing::AtLeast;
using ::testing::_;

namespace {
static void RunMainLoopIteration() {
    std::shared_ptr<GMainLoop> loop(g_main_loop_new(nullptr, false), &g_main_loop_unref);
    auto context = g_main_loop_get_context(loop.get());
    g_main_context_iteration(context, TRUE);
}

class MockWpaSupplicantCommandQueueDelegate : public WpaSupplicantCommandQueue::Delegate {
public:
    MOCK_METHOD1(OnUnsolicitedResponse, void(WpaSupplicantMessage message));
    MOCK_METHOD1(OnWriteMessage, void(WpaSupplicantMessage message));
};
}

TEST(WpaSupplicantCommandQueue, MessageIsWrittenOut) {
    MockWpaSupplicantCommandQueueDelegate mock;
    EXPECT_CALL(mock, OnWriteMessage(_))
            .Times(AtLeast(1));

    WpaSupplicantCommandQueue queue(&mock);
    auto m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");
    queue.EnqueueCommand(m, [=](const WpaSupplicantMessage &msg) {
        EXPECT_TRUE(msg.IsOk());
    });

    RunMainLoopIteration();

    m = WpaSupplicantMessage::CreateRaw("OK");
    queue.HandleMessage(m);
}


TEST(WpaSupplicantCommandQueue, MultipeMessagesWrittenOut) {
    MockWpaSupplicantCommandQueueDelegate mock;
    EXPECT_CALL(mock, OnWriteMessage(_))
            .Times(AtLeast(2));

    WpaSupplicantCommandQueue queue(&mock);

    auto m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");
    queue.EnqueueCommand(m, [=](const WpaSupplicantMessage &msg) {
        EXPECT_TRUE(msg.IsOk());
    });

    m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");
    queue.EnqueueCommand(m, [=](const WpaSupplicantMessage &msg) {
            EXPECT_TRUE(msg.IsFail());
    });

    auto cycleAndRespond = [&](const std::string &data) {
        RunMainLoopIteration();
        auto m = WpaSupplicantMessage::CreateRaw(data);
        queue.HandleMessage(m);
    };

    cycleAndRespond("OK");
    cycleAndRespond("FAIL");
}


TEST(WpaSupplicantCommandQueue, UnsolicitedResponseIsNotified) {
    MockWpaSupplicantCommandQueueDelegate mock;
    EXPECT_CALL(mock, OnUnsolicitedResponse(_))
            .Times(AtLeast(1));

    WpaSupplicantCommandQueue queue(&mock);

    auto m = WpaSupplicantMessage::CreateRaw("<3> P2P-DEVICE-LOST p2p_dev_addr=4e:74:03:70:e2:c1");
    queue.HandleMessage(m);
}

TEST(WpaSupplicantCommandQueue, InvalidMessageNotHandled) {
    MockWpaSupplicantCommandQueueDelegate mock;
    EXPECT_CALL(mock, OnUnsolicitedResponse(_))
            .Times(AtLeast(0));

    WpaSupplicantCommandQueue queue(&mock);
    auto m = WpaSupplicantMessage::CreateRaw("foobar");
    queue.HandleMessage(m);
}



int main(int argc, char** argv) {
  // The following line must be executed to initialize Google Mock
  // (and Google Test) before running the tests.
  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
