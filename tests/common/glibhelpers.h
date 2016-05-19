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

#ifndef GLIBHELPERS_H_
#define GLIBHELPERS_H_

#include <ac/glib_wrapper.h>

#include <chrono>

namespace ac {
namespace testing {

void RunMainLoop(const std::chrono::seconds &seconds);
void RunMainLoopIteration();

} // namespace testing
} // namespace ac

#endif
