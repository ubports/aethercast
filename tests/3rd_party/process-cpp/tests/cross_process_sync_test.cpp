/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include <core/testing/cross_process_sync.h>

#include <core/testing/fork_and_run.h>

#include <gtest/gtest.h>

TEST(CrossProcessSync, signalling_the_sync_object_results_in_correct_count)
{
    core::testing::CrossProcessSync cps;

    auto service = [&cps]()
    {
        for (unsigned int i = 1; i <= 50; i++)
        {
            EXPECT_NO_THROW(cps.try_signal_ready_for(std::chrono::milliseconds{500}));
        }
        return ::testing::Test::HasFailure() ?
            core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    auto client = [&cps]()
    {
        std::uint32_t counter = 0;
        for (unsigned int i = 1; i <= 50; i++)
        {
            EXPECT_NO_THROW(counter = cps.wait_for_signal_ready_for(std::chrono::milliseconds{500}));
            EXPECT_EQ(i, counter);
        }

        return ::testing::Test::HasFailure() ?
            core::posix::exit::Status::failure : core::posix::exit::Status::success;
    };

    EXPECT_EQ(core::testing::ForkAndRunResult::empty, core::testing::fork_and_run(service, client));
}

TEST(CrossProcessSync, timed_out_wait_on_sync_object_throws_correct_exception)
{
    core::testing::CrossProcessSync cps;

    EXPECT_THROW(cps.wait_for_signal_ready_for(std::chrono::milliseconds{1}),
                 core::testing::CrossProcessSync::Error::Timeout);
}
