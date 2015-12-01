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

#include <core/posix/child_process.h>
#include <core/posix/signal.h>

#include <core/testing/fork_and_run.h>

#include <gtest/gtest.h>

TESTP(DeathObserver, construction_and_deconstruction_works,
{
  auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_chld});
  EXPECT_NO_THROW(auto death_observer = core::posix::ChildProcess::DeathObserver::create_once_with_signal_trap(trap));
})

TESTP(DeathObserver, construction_with_a_trap_not_including_sig_chld_throws,
{
    auto trap = core::posix::trap_signals_for_all_subsequent_threads({core::posix::Signal::sig_term});
    EXPECT_ANY_THROW(auto death_observer = core::posix::ChildProcess::DeathObserver::create_once_with_signal_trap(trap));
})
