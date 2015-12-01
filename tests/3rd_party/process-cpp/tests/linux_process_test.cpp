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

#include <core/posix/fork.h>
#include <core/posix/this_process.h>

#include <core/posix/linux/proc/process/stat.h>
#include <core/posix/linux/proc/process/oom_adj.h>
#include <core/posix/linux/proc/process/oom_score.h>
#include <core/posix/linux/proc/process/oom_score_adj.h>

#include <gtest/gtest.h>

#include <map>

TEST(LinuxProcess, accessing_proc_stats_works)
{
    auto child = core::posix::fork(
                [](){ while(true); return core::posix::exit::Status::success;},
                core::posix::StandardStream::empty);

    core::posix::linux::proc::process::Stat stat;
    EXPECT_NO_THROW(child >> stat);
    ASSERT_EQ(core::posix::linux::proc::process::State::running, stat.state);
}

TEST(LinuxProcess, accessing_proc_oom_score_works)
{
    core::posix::linux::proc::process::OomScore oom_score;
    EXPECT_NO_THROW(core::posix::this_process::instance() >> oom_score);
}

TEST(LinuxProcess, accessing_proc_oom_score_adj_works)
{
    core::posix::linux::proc::process::OomScoreAdj oom_score_adj;
    EXPECT_NO_THROW(core::posix::this_process::instance() >> oom_score_adj);
}

namespace
{
// A custom predicate to evaluate values in /proc/pid/oom_score.
// The file contains the oom_score as calculated by the kernel and we are racing
// against the badness heuristics. However, we only accept values within an error
// margin of 10 as the test process does not extensively allocate memory, uses hw devices etc..
::testing::AssertionResult is_approximately_equal(int a, int b)
{
    static const int error_margin = 10;

    if (::abs(a-b) <= 10)
        return ::testing::AssertionSuccess() << ::abs(a-b) << " <= " << error_margin;

    return ::testing::AssertionFailure() << ::abs(a-b) << " > " << error_margin;
}
}

TEST(LinuxProcess, adjusting_proc_oom_score_adj_works)
{
    core::posix::linux::proc::process::OomScoreAdj oom_score_adj
    {
        core::posix::linux::proc::process::OomScoreAdj::max_value()
    };
    EXPECT_NO_THROW(core::posix::this_process::instance() << oom_score_adj);
    EXPECT_NO_THROW(core::posix::this_process::instance() >> oom_score_adj);
    EXPECT_EQ(core::posix::linux::proc::process::OomScoreAdj::max_value(),
              oom_score_adj.value);
    core::posix::linux::proc::process::OomScore oom_score;
    EXPECT_NO_THROW(core::posix::this_process::instance() >> oom_score);
    EXPECT_TRUE(is_approximately_equal(oom_score.value, core::posix::linux::proc::process::OomScoreAdj::max_value()));
}

// For this test we assume that we are not privileged and that the test binary
// does not have CAP_SYS_RESOURCE capabilities.
TEST(LinuxProcess, adjusting_proc_oom_score_adj_to_privileged_values_only_works_if_root)
{
    core::posix::linux::proc::process::OomScoreAdj oom_score_adj
    {
        core::posix::linux::proc::process::OomScoreAdj::min_value()
    };
    EXPECT_NO_THROW(core::posix::this_process::instance() << oom_score_adj);
    EXPECT_NO_THROW(core::posix::this_process::instance() >> oom_score_adj);
    
    // If we are running on virtualized builders or buildds we are running under a fakeroot environment.
    // However, that environment does not give us the required privileges and capabilities to adjust OOM values
    // as we like. At any rate, this check seems to be flaky and we just comment it out.
    // EXPECT_NE(core::posix::linux::proc::process::OomScoreAdj::min_value(),
    //          oom_score_adj.value);
}

TEST(LinuxProcess, trying_to_write_an_invalid_oom_score_adj_throws)
{
    core::posix::linux::proc::process::OomScoreAdj invalid_adj
    {
        core::posix::linux::proc::process::OomScoreAdj::min_value() -1000
    };

    EXPECT_ANY_THROW(core::posix::this_process::instance() << invalid_adj);
}

TEST(LinuxProcess, adjusting_proc_oom_adj_works)
{
    core::posix::linux::proc::process::OomAdj oom_adj
    {
        core::posix::linux::proc::process::OomAdj::max_value()
    };
    EXPECT_NO_THROW(core::posix::this_process::instance() << oom_adj);
    EXPECT_NO_THROW(core::posix::this_process::instance() >> oom_adj);
    EXPECT_EQ(core::posix::linux::proc::process::OomAdj::max_value(),
              oom_adj.value);
    core::posix::linux::proc::process::OomScore oom_score;
    EXPECT_NO_THROW(core::posix::this_process::instance() >> oom_score);
    // This looks weird as we are comparing to OomScoreAdj as opposed to OomAdj.
    // However, /proc/pid/oom_adj is deprecated as of linux 2.6.36 and the value
    // reported in oom_score is in the scale of /proc/pid/oom_score_adj, i.e.,  [-1000, 1000].
    EXPECT_TRUE(is_approximately_equal(oom_score.value, core::posix::linux::proc::process::OomScoreAdj::max_value()));
}

// For this test we assume that we are not privileged and that the test binary
// does not have CAP_SYS_RESOURCE capabilities.
TEST(LinuxProcess, adjusting_proc_oom_adj_to_privileged_values_does_not_work)
{
    core::posix::linux::proc::process::OomAdj oom_adj
    {
        core::posix::linux::proc::process::OomAdj::min_value()
    };
    EXPECT_NO_THROW(core::posix::this_process::instance() << oom_adj);
    EXPECT_NO_THROW(core::posix::this_process::instance() >> oom_adj);

    // If we are running on virtualized builders or buildds we are running under a fakeroot environment.
    // However, that environment does not give us the required privileges and capabilities to adjust OOM values
    // as we like. At any rate, this check seems to be flaky and we just comment it out.
    // EXPECT_NE(core::posix::linux::proc::process::OomAdj::min_value(),
    //          oom_adj.value);
}

TEST(LinuxProcess, trying_to_write_an_invalid_oom_adj_throws)
{
    core::posix::linux::proc::process::OomAdj invalid_adj
    {
        core::posix::linux::proc::process::OomAdj::min_value() - 1000
    };

    EXPECT_ANY_THROW(core::posix::this_process::instance() << invalid_adj);
}
