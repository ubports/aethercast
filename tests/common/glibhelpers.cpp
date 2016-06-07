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

#include <ac/keep_alive.h>

#include "glibhelpers.h"

namespace ac {
namespace testing {

void RunMainLoop(const std::chrono::seconds &seconds) {
    std::shared_ptr<GMainLoop> loop(g_main_loop_new(g_main_context_default(), false), &g_main_loop_unref);
    g_timeout_add_seconds(seconds.count(), [](gpointer user_data) {
        auto loop = static_cast<ac::SharedKeepAlive<GMainLoop>*>(user_data)->ShouldDie();
        g_main_loop_quit(loop.get());
        return FALSE;
    }, new ac::SharedKeepAlive<GMainLoop>{loop});
    g_main_loop_run(loop.get());
}

void RunMainLoopIteration() {
    std::shared_ptr<GMainLoop> loop(g_main_loop_new(g_main_context_default(), false), &g_main_loop_unref);
    auto context = g_main_loop_get_context(loop.get());
    g_main_context_iteration(context, TRUE);
}


} // namespace testing
} // namespace ac
