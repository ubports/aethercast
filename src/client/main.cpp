/*
 *  Copyright (C) 2012  Intel Corporation. All rights reserved.
 *                2015 Canonical, Ltd.
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

#include <functional>

#include <glib.h>
#include <readline/readline.h>
#include <readline/history.h>

extern "C" {
#include "miracastinterface.h"
}

#define PROMPT_OFF "[miracast]# "

GMainLoop *main_loop = nullptr;
GDBusConnection *bus_connection = nullptr;
MiracastInterfaceManager *manager = nullptr;
guint input_source = 0;
GDBusObjectManager *object_manager = nullptr;

void rl_printf(const char *fmt, ...)
{
    va_list args;
    bool save_input;
    char *saved_line;
    int saved_point;

    save_input = !RL_ISSTATE(RL_STATE_DONE);

    if (save_input) {
        saved_point = rl_point;
        saved_line = rl_copy_text(0, rl_end);
        rl_save_prompt();
        rl_replace_line("", 0);
        rl_redisplay();
    }

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    if (save_input) {
        rl_restore_prompt();
        rl_replace_line(saved_line, 0);
        rl_point = saved_point;
        rl_forced_update_display();
        g_free(saved_line);
    }
}

static void cmd_show(const char *arg) {
    if (!manager)
        return;

    auto state = miracast_interface_manager_get_state(manager);
    rl_printf("\tState: %s\n", state);

    auto capabilities = miracast_interface_manager_get_capabilities(manager);
    rl_printf("\tCapabilities:\n");
    for (int n = 0; capabilities[n] != nullptr; n++)
        rl_printf("\t\t%s\n", capabilities[n]);
}

static void scan_done_cb(GObject *object, GAsyncResult *res, gpointer user_data) {
    GError *error = nullptr;
    if (!miracast_interface_manager_call_scan_finish(manager, res, &error)) {
        rl_printf("Failed to scan: %s\n", error->message);
        g_error_free(error);
        return;
    }

    rl_printf("Scan is done\n");
}

static void cmd_scan(const char *arg) {
    if (!manager)
        return;

    miracast_interface_manager_call_scan(manager, nullptr, scan_done_cb, nullptr);
}

static void foreach_device(std::function<void(MiracastInterfaceDevice*)> callback, const char *address = nullptr) {
    if (!callback)
        return;

    auto objects = g_dbus_object_manager_get_objects(object_manager);

    for (auto obj = objects; obj != nullptr; obj = obj->next) {
        auto device = MIRACAST_INTERFACE_OBJECT_PROXY(obj->data);
        if (!device)
            continue;

        auto iface = g_dbus_object_get_interface(G_DBUS_OBJECT(device), "org.wds.Device");
        if (!iface)
            continue;

        auto device_obj = MIRACAST_INTERFACE_DEVICE(iface);

        if (address) {
            auto device_address = miracast_interface_device_get_address(device_obj);
            if (g_strcmp0(device_address, address) != 0)
                continue;
        }

        callback(device_obj);
    }
}

static void cmd_devices(const char *arg) {
    foreach_device([=](MiracastInterfaceDevice *device) {
        auto address = miracast_interface_device_get_address(device);
        auto name = miracast_interface_device_get_name(device);
        rl_printf("Device %s %s\n", address, name);
    });
}

static void cmd_info(const char *arg) {
    if (!arg) {
        rl_printf("No device address supplied\n");
        return;
    }

    bool found = false;

    foreach_device([&](MiracastInterfaceDevice *device) {
        auto address = miracast_interface_device_get_address(device);
        rl_printf("\tAddress: %s\n", address);

        auto name = miracast_interface_device_get_name(device);
        rl_printf("\tName: %s\n", name);

        auto state = miracast_interface_device_get_state(device);
        rl_printf("\tState: %s\n", state);

        auto capabilities = miracast_interface_device_get_capabilities(device);
        rl_printf("\tCapabilities:\n");
        for (int n = 0; capabilities[n] != nullptr; n++)
            rl_printf("\t\t%s\n", capabilities[n]);

        found = true;
    }, arg);

    if (!found)
        rl_printf("Unknown or invalid device address\n");
}

static void device_connect_cb(GObject *object, GAsyncResult *res, gpointer user_data) {
    GError *error = nullptr;
    if (!miracast_interface_device_call_connect_finish(MIRACAST_INTERFACE_DEVICE(object), res, &error)) {
        rl_printf("Failed to connect with device: %s\n", error->message);
        g_error_free(error);
        return;
    }

    rl_printf("Successfully connected with device\n");
}

static void cmd_connect(const char *arg) {
    if (!arg) {
        rl_printf("No device address supplied\n");
        return;
    }

    bool found = false;

    foreach_device([&](MiracastInterfaceDevice *device) {

        miracast_interface_device_call_connect(device, "sink", nullptr,
                                               device_connect_cb, nullptr);

        found = true;
    }, arg);

    if (!found)
        rl_printf("Unknown or invalid device address\n");
}

static void device_disconnect_cb(GObject *object, GAsyncResult *res, gpointer user_data) {
    GError *error = nullptr;
    if (!miracast_interface_device_call_disconnect_finish(MIRACAST_INTERFACE_DEVICE(object), res, &error)) {
        rl_printf("Failed to disconnect from device: %s\n", error->message);
        g_error_free(error);
        return;
    }

    rl_printf("Successfully disconnected from device\n");
}

static void cmd_disconnect(const char *arg) {
    if (!arg) {
        rl_printf("No device address supplied\n");
        return;
    }

    bool found = false;

    foreach_device([&](MiracastInterfaceDevice *device) {

        miracast_interface_device_call_disconnect(device, nullptr,
                                               device_disconnect_cb, nullptr);

        found = true;
    }, arg);

    if (!found)
        rl_printf("Unknown or invalid device address\n");
}

static const struct {
    const char *cmd;
    const char *arg;
    void (*func) (const char *arg);
    const char *desc;
} cmd_table[] = {
    { "show", NULL, cmd_show, "Show manager properties" },
    { "scan", NULL, cmd_scan, "Scan for devices" },
    { "devices", NULL, cmd_devices, "List available devices" },
    { "info", "<address>", cmd_info, "Display detailed information about a device" },
    { "connect", "<address>", cmd_connect, "Connect to a device" },
    { "disconnect", "<address>", cmd_disconnect, "Disconnect from a device" },
    { NULL }
};

static void rl_handler(char *input) {
    char *cmd, *arg;
    int i;

    if (!input) {
        rl_insert_text("quit");
        rl_redisplay();
        rl_crlf();
        g_main_loop_quit(main_loop);
        return;
    }

    cmd = strtok_r(input, " ", &arg);
    if (!cmd)
        goto done;

    if (arg) {
        int len = strlen(arg);
        if (len > 0 && arg[len - 1] == ' ')
            arg[len - 1] = '\0';
    }

    for (i = 0; cmd_table[i].cmd; i++) {
        if (strcmp(cmd, cmd_table[i].cmd))
            continue;

        if (cmd_table[i].func) {
            cmd_table[i].func(arg);
            goto done;
        }
    }

    if (strcmp(cmd, "help")) {
        printf("Invalid command\n");
        goto done;
    }

    printf("Available commands:\n");

    for (i = 0; cmd_table[i].cmd; i++) {
        if (cmd_table[i].desc)
            printf(" %s %-*s %s\n", cmd_table[i].cmd,
                   (int)(25 - strlen(cmd_table[i].cmd)),
                   cmd_table[i].arg ? : "",
                   cmd_table[i].desc ? : "");
    }

done:
    g_free(input);
}

static gboolean input_handler(GIOChannel *channel, GIOCondition condition,
                              gpointer user_data) {
    if (condition & G_IO_IN) {
        rl_callback_read_char();
        return TRUE;
    }

    if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
        g_main_loop_quit(main_loop);
        return FALSE;
    }

    return TRUE;
}

static void setup_standard_input(void) {
    GIOChannel *channel;

    channel = g_io_channel_unix_new(fileno(stdin));

    input_source = g_io_add_watch(channel,
                            (GIOCondition) (G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL),
                            input_handler, nullptr);

    g_io_channel_unref(channel);

}

void device_added_cb(GDBusObjectManager *manager, GDBusObject *object, gpointer user_data) {
    rl_printf("Device added\n");
}

void device_removed_cb(GDBusObjectManager *manager, GDBusObject *object, gpointer user_data) {
    rl_printf("Device removed\n");
}

void object_manager_created_cb(GObject *object, GAsyncResult *res, gpointer user_data) {
    GError *error = nullptr;
    object_manager = miracast_interface_object_manager_client_new_finish(res, &error);
    if (!object_manager) {
        g_error_free(error);
        return;
    }

    g_signal_connect(object_manager, "object-added", G_CALLBACK(device_added_cb), nullptr);
    g_signal_connect(object_manager, "object-removed", G_CALLBACK(device_removed_cb), nullptr);

    setup_standard_input();
}

static void manager_property_changed_cb(GObject *gobject, GParamSpec *pspec, gpointer user_data) {
    char *name = (char*) user_data;

    if (g_strcmp0(name, "state") == 0) {
        auto state = miracast_interface_manager_get_state(manager);
        rl_printf("[CHG] Manger State: %s", state);
    }
}

void manager_connected_cb(GObject *object, GAsyncResult *res, gpointer user_data) {
    GError *error = nullptr;
    manager = miracast_interface_manager_proxy_new_finish(res, &error);
    if (!manager) {
        printf("ERROR: Could not connect with manager: %s\n", error->message);
        g_error_free(error);
        g_main_loop_quit(main_loop);
        return;
    }

    g_signal_connect(G_OBJECT(manager), "notify::state", G_CALLBACK(manager_property_changed_cb), (gpointer) "state");

    // Use a high enough timeout to make sure we get the end of the scan
    // method call which has an internal timeout of 30 seconds
    g_dbus_proxy_set_default_timeout(G_DBUS_PROXY(manager), 60 * 1000);

    miracast_interface_object_manager_client_new(bus_connection,
                             G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                             "org.wds", "/org/wds",
                             nullptr, object_manager_created_cb, nullptr);

    setup_standard_input();
}

void service_found_cb(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data) {
    miracast_interface_manager_proxy_new(bus_connection, G_DBUS_PROXY_FLAGS_NONE,
                                         "org.wds", "/org/wds", nullptr, manager_connected_cb,
                                         nullptr);
}

void service_lost_cb(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    if (input_source > 0) {
        g_source_remove(input_source);
        input_source = 0;
    }

    if (manager)
        g_object_unref(manager);
}

int main(int argc, char **argv) {
    main_loop = g_main_loop_new(nullptr, FALSE);

    GError *error = nullptr;
    bus_connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!bus_connection) {
        g_warning("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return -1;
    }

    rl_erase_empty_line = 1;
    rl_callback_handler_install(nullptr, rl_handler);

    rl_set_prompt(PROMPT_OFF);
    rl_redisplay();

    g_bus_watch_name_on_connection(bus_connection, "org.wds",
                                   G_BUS_NAME_WATCHER_FLAGS_NONE,
                                   service_found_cb,
                                   service_lost_cb,
                                   nullptr, nullptr);

    g_main_loop_run(main_loop);

    rl_message();
    rl_callback_handler_remove();

    g_object_unref(bus_connection);
    g_main_loop_unref(main_loop);

    if (manager)
        g_object_unref(manager);

    if (input_source > 0)
        g_source_remove(input_source);

    return 0;
}
