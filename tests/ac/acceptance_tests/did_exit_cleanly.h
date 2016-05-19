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

#ifndef DID_EXIT_CLEANLY_H_
#define DID_EXIT_CLEANLY_H_

#include <gtest/gtest.h>

#include <core/posix/child_process.h>
#include <core/posix/wait.h>

namespace testing {
AssertionResult DidExitCleanly(core::posix::ChildProcess& child);
AssertionResult DidExitCleanly(const core::posix::wait::Result& result);
}

#endif
