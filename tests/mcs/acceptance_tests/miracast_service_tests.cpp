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

#include "did_exit_cleanly.h"

namespace {
class MiracastService : public ::testing::TestWithParam<core::posix::Signal> {
};
}

TEST_P(MiracastService, ExitsCleanlyForSigIntAndSigTerm) {
    auto service = core::posix::fork([]() {
        auto result = mcs::MiracastService::Main(mcs::MiracastService::MainOptions{false, false});
        return static_cast<core::posix::exit::Status>(result);
    }, core::posix::StandardStream::empty);

    std::this_thread::sleep_for(std::chrono::milliseconds{1000});

    service.send_signal_or_throw(GetParam());
    EXPECT_TRUE(testing::DidExitCleanly(service));
}

INSTANTIATE_TEST_CASE_P(ShutdownBehavior,
                        MiracastService,
                        ::testing::Values(core::posix::Signal::sig_int, core::posix::Signal::sig_term));
