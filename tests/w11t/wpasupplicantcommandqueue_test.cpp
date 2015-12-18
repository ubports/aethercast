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

#include <w11t/commandqueue.h>

using ::testing::AtLeast;
using ::testing::_;

namespace {
static void RunMainLoopIteration() {
    std::shared_ptr<GMainLoop> loop(g_main_loop_new(nullptr, false), &g_main_loop_unref);
    auto context = g_main_loop_get_context(loop.get());
    g_main_context_iteration(context, TRUE);
}

class CommandQueueDelegate : public w11t::CommandQueue::Delegate {
public:
    MOCK_METHOD1(OnUnsolicitedResponse, void(w11t::Message message));
    MOCK_METHOD1(OnWriteMessage, void(w11t::Message message));
};

struct CommandQueueFixture : public ::testing::Test {
    void CycleAndRespond(w11t::CommandQueue &queue, const std::string &data) {
        RunMainLoopIteration();
        auto m = w11t::Message::Parse(data);
        queue.HandleMessage(m);
    }
};
}

TEST_F(CommandQueueFixture, MessageIsWrittenOut) {
    CommandQueueDelegate mock;
    EXPECT_CALL(mock, OnWriteMessage(_))
            .Times(AtLeast(1));

    w11t::CommandQueue queue(&mock);
    auto m = w11t::Message::CreateRequest("P2P_CONNECT");
    queue.EnqueueCommand(m, [=](const w11t::Message &msg) {
        EXPECT_TRUE(msg.IsOk());
    });

    CycleAndRespond(queue, "OK");
}


TEST_F(CommandQueueFixture, MultipeMessagesWrittenOut) {
    CommandQueueDelegate mock;
    EXPECT_CALL(mock, OnWriteMessage(_))
            .Times(AtLeast(2));

    w11t::CommandQueue queue(&mock);

    auto m = w11t::Message::CreateRequest("P2P_CONNECT");
    queue.EnqueueCommand(m, [=](const w11t::Message &msg) {
        EXPECT_TRUE(msg.IsOk());
    });

    m = w11t::Message::CreateRequest("P2P_CONNECT");
    queue.EnqueueCommand(m, [=](const w11t::Message &msg) {
            EXPECT_TRUE(msg.IsFail());
    });


    CycleAndRespond(queue, "OK");
    CycleAndRespond(queue, "FAIL");
}


TEST(CommandQueue, UnsolicitedResponseIsNotified) {
    CommandQueueDelegate mock;
    EXPECT_CALL(mock, OnUnsolicitedResponse(_))
            .Times(AtLeast(1));

    w11t::CommandQueue queue(&mock);

    auto m = w11t::Message::Parse("<3> P2P-DEVICE-LOST p2p_dev_addr=4e:74:03:70:e2:c1");
    queue.HandleMessage(m);
}

TEST(CommandQueue, InvalidMessageNotHandled) {
    CommandQueueDelegate mock;
    EXPECT_CALL(mock, OnUnsolicitedResponse(_))
            .Times(AtLeast(0));

    w11t::CommandQueue queue(&mock);
    auto m = w11t::Message::Parse("foobar");
    queue.HandleMessage(m);
}
