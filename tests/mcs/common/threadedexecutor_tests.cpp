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

#include <gmock/gmock.h>

#include "mcs/common/executable.h"
#include "mcs/common/threadedexecutor.h"

using namespace ::testing;

namespace {
class MockExecutable : public mcs::common::Executable {
public:
    MOCK_METHOD0(Stop, bool());
    MOCK_METHOD0(Start, bool());
    MOCK_METHOD0(Execute, bool());

    std::string Name() const override {
        return "MockExecutable";
    }
};
}

TEST(ThreadedExecutor, CorrectStartAndStopBehaviour) {
    auto executable = std::make_shared<MockExecutable>();

    EXPECT_CALL(*executable, Start())
            .Times(1)
            .WillRepeatedly(Return(true));
    EXPECT_CALL(*executable, Stop())
            .Times(1)
            .WillRepeatedly(Return(true));

    EXPECT_CALL(*executable, Execute())
            .Times(AtLeast(0))
            .WillRepeatedly(Return(true));

    const auto executor = std::make_shared<mcs::common::ThreadedExecutor>(executable);

    EXPECT_FALSE(executor->Running());
    EXPECT_FALSE(executor->Stop());
    EXPECT_TRUE(executor->Start());
    EXPECT_TRUE(executor->Running());
    EXPECT_FALSE(executor->Start());
    EXPECT_TRUE(executor->Stop());
    EXPECT_FALSE(executor->Stop());
}

TEST(ThreadedExecutor, CorrectlyExecutes) {
    auto executable = std::make_shared<MockExecutable>();

    unsigned int count = 0;
    bool running = true;

    EXPECT_CALL(*executable, Start())
            .Times(1)
            .WillRepeatedly(Return(true));
    EXPECT_CALL(*executable, Stop())
            .Times(1)
            .WillRepeatedly(Return(true));

    EXPECT_CALL(*executable, Execute())
            .Times(AtLeast(10))
            .WillRepeatedly(DoAll(Invoke([&]() { count++; running = count < 10; }), ReturnPointee(&running)));

    const auto executor = std::make_shared<mcs::common::ThreadedExecutor>(executable);

    EXPECT_TRUE(executor->Start());

    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    EXPECT_TRUE(executor->Stop());

    EXPECT_EQ(10, count);
}

TEST(ThreadedExecutor, ExecutableFailsToStart) {
    auto executable = std::make_shared<MockExecutable>();

    EXPECT_CALL(*executable, Start())
            .Times(1)
            .WillOnce(Return(false));

    const auto executor = std::make_shared<mcs::common::ThreadedExecutor>(executable);

    EXPECT_FALSE(executor->Start());
    EXPECT_FALSE(executor->Running());
}

TEST(ThreadedExecutor, IgnoresFailedStopFromExecutable) {
    auto executable = std::make_shared<MockExecutable>();

    EXPECT_CALL(*executable, Start())
            .Times(1)
            .WillOnce(Return(true));

    EXPECT_CALL(*executable, Stop())
            .Times(1)
            .WillOnce(Return(false));

    const auto executor = std::make_shared<mcs::common::ThreadedExecutor>(executable);

    EXPECT_TRUE(executor->Start());
    EXPECT_TRUE(executor->Stop());
    EXPECT_FALSE(executor->Running());
}
