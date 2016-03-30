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

#include <gtest/gtest.h>

#include "mcs/common/threadedexecutorfactory.h"
#include "mcs/common/threadedexecutor.h"

TEST(ThreadedExecutorFactory, ConstructsCorrectType) {
    const auto factory = std::make_shared<mcs::common::ThreadedExecutorFactory>();
    mcs::common::Executable::Ptr null_executable = nullptr;
    EXPECT_TRUE(!!dynamic_cast<mcs::common::ThreadedExecutor*>(factory->Create(null_executable).get()));
}
