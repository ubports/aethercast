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

#ifndef FIRMWARELOADER_H_
#define FIRMWARELOADER_H_

#include <glib.h>
#include <gio/gio.h>

#include <string>
#include <boost/noncopyable.hpp>

namespace w11tng {
class WiFiFirmwareLoader {
public:
    class Delegate : private boost::noncopyable {
    public:
        virtual void OnFirmwareLoaded() = 0;
        virtual void OnFirmwareUnloaded() = 0;
    };

    WiFiFirmwareLoader(const std::string &interface_name, Delegate *delegate);
    ~WiFiFirmwareLoader();

    void SetInterfaceName(const std::string &interface_name);

    bool IsNeeded();
    bool TryLoad();

private:
    static gboolean OnRetryLoad(gpointer user_data);
    static void OnInterfaceFirmwareSet(GDBusConnection *conn, GAsyncResult *res, gpointer user_data);

private:
    std::string interface_name_;
    Delegate *delegate_;
    guint reload_timeout_source_;
};
} // namespace w11tng

#endif
