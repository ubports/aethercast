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

#ifndef W11TNG_DHCP_LISTENER_SKELETON_H_
#define W11TNG_DHCP_LISTENER_SKELETON_H_

#include <gio/gio.h>

#include <memory>
#include <map>

#include <mcs/non_copyable.h>
#include <mcs/scoped_gobject.h>
#include <mcs/shared_gobject.h>

namespace w11tng {
class DhcpListenerSkeleton : public std::enable_shared_from_this<DhcpListenerSkeleton> {
public:
    typedef std::shared_ptr<DhcpListenerSkeleton> Ptr;

    class Delegate : public mcs::NonCopyable {
    public:
        virtual void OnNewConnection() = 0;
        virtual void OnConnectionClosed() = 0;
        virtual void OnEvent(const std::map<std::string, std::string> &properties) = 0;
    };

    static Ptr Create(const std::string &path, std::weak_ptr<Delegate> delegate);

    ~DhcpListenerSkeleton();

    std::string Address() const;

private:
    DhcpListenerSkeleton(std::weak_ptr<Delegate> delegate);
    Ptr FinalizeConstruction(const std::string &path);

    void ConnectSignals();

private:
    static gboolean OnAuthorizePeer(GDBusAuthObserver *observer, GIOStream *stream,
                                    GCredentials *credentials, gpointer user_data);
    static gboolean OnNewConnection(GDBusServer *server, GDBusConnection *connection, gpointer user_data);
    static void OnConnectionClosed(GDBusConnection *connection, gboolean remote_peer_vanished,
                                   GError *error, gpointer user_data);
    static void OnEvent(GDBusConnection *connection, const char *sender_name, const char *object_path,
                        const char *interface_name, const char *signal_name, GVariant *parameters,
                        gpointer user_data);

private:
    std::weak_ptr<Delegate> delegate_;
    std::string address_;
    mcs::ScopedGObject<GDBusServer> server_;
    mcs::ScopedGObject<GDBusAuthObserver> auth_observer_;
    mcs::ScopedGObject<GDBusConnection> current_connection_;
};
} // namespace w11tng

#endif
