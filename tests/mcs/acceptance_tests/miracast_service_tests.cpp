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

#include <thread>

#include <mcs/miracastservice.h>
#include <wds/sink.h>

#include <gtest/gtest.h>

#include <core/posix/fork.h>

namespace {
::testing::AssertionResult DidFinishSuccessfully(const core::posix::wait::Result& result)
{
    if (result.status != core::posix::wait::Result::Status::exited)
        return ::testing::AssertionFailure() << "Process did not exit, but: " << (int)result.status;
    if (result.detail.if_exited.status != core::posix::exit::Status::success)
        return ::testing::AssertionFailure() << "Process did exit with failure.";

    return ::testing::AssertionSuccess();
}
}

TEST(MiracastService, ExitsCleanlyForSigIntAndSigTerm) {
    auto service = core::posix::fork([]() {
        auto result = mcs::MiracastService::Main(mcs::MiracastService::MainOptions{false, false});
        return static_cast<core::posix::exit::Status>(result);
    }, core::posix::StandardStream::empty);

    std::this_thread::sleep_for(std::chrono::milliseconds{250});

    service.send_signal_or_throw(core::posix::Signal::sig_int);
    EXPECT_TRUE(DidFinishSuccessfully(service.wait_for(core::posix::wait::Flags::untraced)));
}
