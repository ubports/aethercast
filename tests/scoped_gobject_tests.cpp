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

#include <mcs/scoped_gobject.h>

#include <gio/gio.h>

#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

TEST(ScopedGObject, BehavesNicelyOnNullInstances)
{
    mcs::ScopedGObject<GSubprocess> obj;
}

TEST(ScopedGObject, UnrefsObjectsOnDestruction)
{
    struct State {
        std::mutex guard;
        std::condition_variable cv;
        bool cleaned_up_{false};
    } state;

    auto notify = [](gpointer data, GObject *, gboolean) {
        auto state = static_cast<State*>(data);
        std::unique_lock<std::mutex> ul{state->guard};
        state->cleaned_up_ = true;
        state->cv.notify_all();
    };

    auto sleep = g_subprocess_new(G_SUBPROCESS_FLAGS_NONE, nullptr, "/bin/sleep", "5", nullptr);
    g_object_add_toggle_ref(G_OBJECT(sleep), notify, &state);

    // Reference count is 2 when entering this scope.
    // That is, after having left the scope, we expect the
    // the reference count to be 1.
    {
        mcs::ScopedGObject<GSubprocess> obj{sleep};
    }

    g_subprocess_force_exit(sleep);
    g_object_unref(sleep);

    std::unique_lock<std::mutex> ul{state.guard};
    EXPECT_TRUE(state.cv.wait_for(ul, std::chrono::seconds{5}, [&state]() { return state.cleaned_up_; }));
}
