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

#ifndef W11TNG_FILEMONITOR_H_
#define W11TNG_FILEMONITOR_H_

#include <gio/gio.h>

#include <memory>
#include <map>
#include <string>

#include <mcs/non_copyable.h>
#include <mcs/scoped_gobject.h>

namespace w11tng {

class FileMonitor : public std::enable_shared_from_this<FileMonitor> {
public:
    typedef std::shared_ptr<FileMonitor> Ptr;

    class Delegate : public mcs::NonCopyable {
    public:
        virtual void OnFileChanged(const std::string &path) = 0;
    };

    static Ptr Create(const std::string &path, const std::weak_ptr<Delegate> &delegate);

    ~FileMonitor();

private:
    FileMonitor(const std::weak_ptr<Delegate> &delegate);
    Ptr FinalizeConstruction(const std::string &path);

private:
    static void OnChanged(GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event_type, gpointer user_data);

private:
    std::weak_ptr<Delegate> delegate_;
    std::map<std::string,void*> watches_;
    mcs::ScopedGObject<GFileMonitor> monitor_;
};

} // namespace w11tng

#endif
