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

#include <sys/prctl.h>

#include <mcs/logger.h>
#include <mcs/keep_alive.h>

#include "processexecutor.h"

namespace w11tng {

ProcessExecutor::Ptr ProcessExecutor::Create(const std::string &path, const std::vector<std::string> &argv,
                                             const std::weak_ptr<Delegate> &delegate) {
    return std::shared_ptr<ProcessExecutor>(new ProcessExecutor(delegate))->FinalizeConstruction(path, argv);
}

ProcessExecutor::Ptr ProcessExecutor::FinalizeConstruction(const std::string &path, const std::vector<std::string> &argv) {
    auto sp = shared_from_this();

    auto arguments = g_ptr_array_new();
    g_ptr_array_add(arguments, (gpointer) path.c_str());
    for (auto arg : argv)
        g_ptr_array_add(arguments, (gpointer) g_strdup(arg.c_str()));
    g_ptr_array_add(arguments, nullptr);

    auto cmdline = g_strjoinv(" ", reinterpret_cast<gchar**>(arguments->pdata));
    MCS_DEBUG("Running with: %s", cmdline);
    g_free(cmdline);

    GError *error = nullptr;
    if (!g_spawn_async(nullptr, reinterpret_cast<gchar**>(arguments->pdata), nullptr,
                       GSpawnFlags(G_SPAWN_DO_NOT_REAP_CHILD),
                       [](gpointer user_data) { prctl(PR_SET_PDEATHSIG, SIGKILL); }, nullptr,
                       &pid_, &error)) {

        MCS_ERROR("Failed to spawn DHCP server: %s", error->message);
        g_error_free(error);
        g_ptr_array_free(arguments, FALSE);
        return sp;
    }

    g_ptr_array_free(arguments, FALSE);

    process_watch_ = g_child_watch_add_full(0, pid_, [](GPid pid, gint status, gpointer user_data) {
        auto thiz = static_cast<mcs::WeakKeepAlive<ProcessExecutor>*>(user_data)->GetInstance().lock();

        if (!WIFEXITED(status))
            MCS_WARNING("DHCP server (pid %d) exited with status %d", pid, status);
        else
            MCS_DEBUG("DHCP server successfully terminated");

        if (not thiz)
            return;

        thiz->pid_ = -1;

        g_source_remove(thiz->process_watch_);

        if (auto sp = thiz->delegate_.lock())
            sp->OnProcessTerminated();
    }, new mcs::WeakKeepAlive<ProcessExecutor>(shared_from_this()),
    [](gpointer data) { delete static_cast<mcs::WeakKeepAlive<ProcessExecutor>*>(data); });


    return sp;
}

ProcessExecutor::ProcessExecutor(const std::weak_ptr<Delegate> &delegate) :
    delegate_(delegate),
    pid_(-1),
    process_watch_(0) {
}

ProcessExecutor::~ProcessExecutor() {
    if (pid_ <= 0)
        return;

    ::kill(pid_, SIGTERM);
    g_spawn_close_pid(pid_);

    pid_ = -1;

    if (process_watch_ > 0)
        g_source_remove(process_watch_);
}

} // namespace w11tng
