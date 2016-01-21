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

#include <mcs/keep_alive.h>

#include "filemonitor.h"

namespace w11tng {
FileMonitor::Ptr FileMonitor::Create(const std::string &path, const std::weak_ptr<Delegate> &delegate) {
    return std::shared_ptr<FileMonitor>(new FileMonitor(delegate))->FinalizeConstruction(path);
}

FileMonitor::Ptr FileMonitor::FinalizeConstruction(const std::string &path) {
    auto sp = shared_from_this();

    auto file_to_watch = g_file_new_for_path(path.c_str());
    if (!file_to_watch) {
        MCS_ERROR("Failed to access file at path %s", path);
        return sp;
    }

    GError *error = nullptr;
    monitor_.reset(g_file_monitor_file(file_to_watch, G_FILE_MONITOR_NONE, nullptr, &error));
    if (error) {
        MCS_ERROR("Failed to create monitor for file %s: %s", path, error->message);
        g_error_free(error);
        return sp;
    }

    g_signal_connect_data(monitor_.get(), "changed", G_CALLBACK(&FileMonitor::OnChanged),
                          new mcs::WeakKeepAlive<FileMonitor>(shared_from_this()),
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<FileMonitor>*>(data); },
                          GConnectFlags(0));

    return sp;
}

FileMonitor::FileMonitor(const std::weak_ptr<Delegate> &delegate) :
    delegate_(delegate) {
}

FileMonitor::~FileMonitor() {
    g_file_monitor_cancel(monitor_.get());
}

void FileMonitor::OnChanged(GFileMonitor *monitor, GFile *file, GFile *other_file,
                                GFileMonitorEvent event_type, gpointer user_data) {

    auto thiz = static_cast<mcs::WeakKeepAlive<FileMonitor>*>(user_data)->GetInstance().lock();

    if (not thiz)
        return;

    if (event_type != G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT)
        return;

    if (auto sp = thiz->delegate_.lock())
        sp->OnFileChanged(g_file_get_path(file));
}
} // namespace w11tng
