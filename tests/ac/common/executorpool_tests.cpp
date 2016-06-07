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

#include "ac/common/executorpool.h"

using namespace ::testing;

namespace {
class MockExecutable : public ac::common::Executable {
public:
    MOCK_METHOD0(Start, bool());
    MOCK_METHOD0(Stop, bool());
    MOCK_METHOD0(Execute, bool());

    std::string Name() const override {
        return "MockExecutable";
    }
};

class MockExecutor : public ac::common::Executor {
public:
    MOCK_METHOD0(Start, bool());
    MOCK_METHOD0(Stop, bool());
    MOCK_CONST_METHOD0(Running, bool());
};

class MockExecutorFactory : public ac::common::ExecutorFactory {
public:
    MOCK_METHOD1(Create, ac::common::Executor::Ptr(const ac::common::Executable::Ptr&));
};
}

TEST(ExecutorPool, PoolSizeIsRespected) {
    auto factory = std::make_shared<MockExecutorFactory>();

    EXPECT_CALL(*factory, Create(_))
            .Times(1)
            .WillRepeatedly(Return(std::make_shared<MockExecutor>()));

    ac::common::ExecutorPool pool(factory, 1);

    EXPECT_TRUE(pool.Add(std::make_shared<MockExecutable>()));
    EXPECT_FALSE(pool.Add(std::make_shared<MockExecutable>()));
}

TEST(ExecutorPool, StartAndStopRefuseInInvalidTransition) {
    auto factory = std::make_shared<MockExecutorFactory>();
    ac::common::ExecutorPool pool(factory, 1);

    EXPECT_FALSE(pool.Stop());
    EXPECT_TRUE(pool.Start());
    EXPECT_FALSE(pool.Start());
    EXPECT_TRUE(pool.Stop());
    EXPECT_FALSE(pool.Stop());
}

TEST(ExecutorPool, StartAndStopsAllExecutors) {
    auto factory = std::make_shared<MockExecutorFactory>();
    auto executor = std::make_shared<MockExecutor>();

    EXPECT_CALL(*executor, Start())
            .Times(2)
            .WillRepeatedly(Return(true));

    EXPECT_CALL(*executor, Stop())
            .Times(2)
            .WillRepeatedly(Return(true));

    EXPECT_CALL(*factory, Create(_))
            .Times(2)
            .WillRepeatedly(Return(executor));

    ac::common::ExecutorPool pool(factory, 2);

    EXPECT_TRUE(pool.Add(std::make_shared<MockExecutable>()));
    EXPECT_TRUE(pool.Add(std::make_shared<MockExecutable>()));

    EXPECT_TRUE(pool.Start());
    EXPECT_TRUE(pool.Stop());
}

TEST(ExecutorPool, RecoversFromFailedExecutorStart) {
    auto factory = std::make_shared<MockExecutorFactory>();

    auto executor0 = std::make_shared<MockExecutor>();

    EXPECT_CALL(*executor0, Start())
            .Times(1)
            .WillOnce(Return(true));

    EXPECT_CALL(*executor0, Running())
            .Times(1)
            .WillOnce(Return(true));

    EXPECT_CALL(*executor0, Stop())
            .Times(1)
            .WillOnce(Return(true));

    auto executor1 = std::make_shared<MockExecutor>();

    EXPECT_CALL(*executor1, Start())
            .Times(1)
            .WillOnce(Return(false));

    EXPECT_CALL(*executor1, Running())
            .Times(1)
            .WillOnce(Return(false));

    EXPECT_CALL(*executor1, Stop())
            .Times(0);

    ac::common::Executable::Ptr executable0 = std::make_shared<MockExecutable>();

    EXPECT_CALL(*factory, Create(executable0))
            .Times(1)
            .WillOnce(Return(executor0));

    ac::common::Executable::Ptr executable1 = std::make_shared<MockExecutable>();

    EXPECT_CALL(*factory, Create(executable1))
            .Times(1)
            .WillOnce(Return(executor1));

    ac::common::ExecutorPool pool(factory, 2);

    EXPECT_TRUE(pool.Add(executable0));
    EXPECT_TRUE(pool.Add(executable1));

    EXPECT_FALSE(pool.Start());
    EXPECT_FALSE(pool.Stop());
}

TEST(ExecutorPool, OnlyStopsWhenAllExecutorsAreStopped) {
    auto factory = std::make_shared<MockExecutorFactory>();

    auto executor0 = std::make_shared<MockExecutor>();

    EXPECT_CALL(*executor0, Start())
            .Times(1)
            .WillOnce(Return(true));

    EXPECT_CALL(*executor0, Stop())
            // Will be called twice as the d'tor will try to stop the
            // executor too when it is still running on destruction
            .Times(2)
            .WillRepeatedly(Return(true));

    auto executor1 = std::make_shared<MockExecutor>();

    EXPECT_CALL(*executor1, Start())
            .Times(1)
            .WillOnce(Return(true));

    EXPECT_CALL(*executor1, Stop())
            // Will be called twice as the d'tor will try to stop the
            // executor too when it is still running on destruction
            .Times(2)
            .WillOnce(Return(false))
            .WillOnce(Return(true));

    ac::common::Executable::Ptr executable0 = std::make_shared<MockExecutable>();

    EXPECT_CALL(*factory, Create(executable0))
            .Times(1)
            .WillOnce(Return(executor0));

    ac::common::Executable::Ptr executable1 = std::make_shared<MockExecutable>();

    EXPECT_CALL(*factory, Create(executable1))
            .Times(1)
            .WillOnce(Return(executor1));

    ac::common::ExecutorPool pool(factory, 2);

    EXPECT_TRUE(pool.Add(executable0));
    EXPECT_TRUE(pool.Add(executable1));

    EXPECT_TRUE(pool.Start());
    EXPECT_FALSE(pool.Stop());
    EXPECT_TRUE(pool.Running());
}
