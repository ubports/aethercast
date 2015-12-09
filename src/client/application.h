/*
 *  Copyright (C) 2015 Canonical, Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <map>
#include <functional>

#include <glib.h>

extern "C" {
#include "miracastinterface.h"
}

namespace mcs {
namespace client {

class Application {
public:
    struct MainOptions {
        static MainOptions FromCommandLine(int argc, char** argv);
    };

    static int Main(const MainOptions &options);

    Application();
    ~Application();

    int Run();

private:
    struct Command {
        std::string name;
        std::string arguments;
        std::string description;
        std::function<void(std::string)> func;
    };

    void HandleShowCommand(const std::string &arguments);
    void HandleScanCommand(const std::string &arguments);
    void HandleDevicesCommand(const std::string &arguments);
    void HandleInfoCommand(const std::string &arguments);
    void HandleConnectCommand(const std::string &arguments);
    void HandleDisconnectCommand(const std::string &arguments);

    void RegisterCommand(const Command &command);

private:
    static gboolean OnUserInput(GIOChannel *channel, GIOCondition condition, gpointer user_data);
    static void OnReadlineMessage(char *input);
    static void OnServiceLost(GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void OnServiceFound(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data);
    static void OnManagerConnected(GObject *object, GAsyncResult *res, gpointer user_data);
    static void OnDeviceAdded(GDBusObjectManager *manager, GDBusObject *object, gpointer user_data);
    static void OnDeviceRemoved(GDBusObjectManager *manager, GDBusObject *object, gpointer user_data);
    static void OnObjectManagerCreated(GObject *object, GAsyncResult *res, gpointer user_data);

    static void OnManagerPropertiesChanged(GDBusConnection *connection,
                                           const gchar *sender_name,
                                           const gchar *object_path,
                                           const gchar *interface_name,
                                           const gchar *signal_name,
                                           GVariant *parameters,
                                           gpointer user_data);

    static void OnScanDone(GObject *object, GAsyncResult *res, gpointer user_data);
    static void OnDeviceConnected(GObject *object, GAsyncResult *res, gpointer user_data);
    static void OnDeviceDisconnected(GObject *object, GAsyncResult *res, gpointer user_data);
private:
    void SetupStandardInput();
    void ForeachDevice(std::function<void(MiracastInterfaceDevice*)> callback, const std::string &address_filter = "");

private:
    GDBusConnection *bus_connection_;
    MiracastInterfaceManager *manager_;
    guint input_source_;
    GDBusObjectManager *object_manager_;

private:
    // Needs to be static otherwise we can't access it from the readline
    // callback function (which sadly doesn't allow passing any user data)
    static GMainLoop *main_loop_;
    static std::map<std::string,Command> available_commands_;
};

} // namespace client
} // namespace mcs

#endif
