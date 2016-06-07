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

#include <memory>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>

#include <readline/readline.h>
#include <readline/history.h>

#include "ac/glib_wrapper.h"

#include "application.h"

using namespace std::placeholders;

namespace {
const unsigned int kDBusTimeout = 120;

class PromptSaver {
public:
    PromptSaver() {
        save_input_ = !RL_ISSTATE(RL_STATE_DONE);
        if (save_input_) {
            saved_point_ = rl_point;
            saved_line_ = rl_copy_text(0, rl_end);
            rl_save_prompt();
            rl_replace_line("", 0);
            rl_redisplay();
        }
    }

    ~PromptSaver() {
        if (save_input_) {
            rl_restore_prompt();
            rl_replace_line(saved_line_, 0);
            rl_point = saved_point_;
            rl_redisplay();
            g_free(saved_line_);
        }
    }

private:
    bool save_input_;
    int saved_point_;
    char *saved_line_;
};
}

namespace ac {
namespace client {

GMainLoop *Application::main_loop_ = nullptr;
std::map<std::string,Application::Command> Application::available_commands_ = { };

Application::MainOptions Application::MainOptions::FromCommandLine(int argc, char** argv) {

    static GOptionEntry options[] = {
        { NULL },
    };

    std::shared_ptr<GOptionContext> context{g_option_context_new(nullptr), [](GOptionContext *ctxt) { g_option_context_free(ctxt); }};
    GError *error = nullptr;

    g_option_context_add_main_entries(context.get(), options, NULL);

    if (!g_option_context_parse(context.get(), &argc, &argv, &error)) {
        if (error) {
            std::cerr << error->message << std::endl;
            g_error_free(error);
        } else
            std::cerr << "An unknown error occured" << std::endl;
        exit(1);
    }

    return MainOptions{};
}

int Application::Main(const MainOptions &options) {
    Application app;
    return app.Run();
}

Application::Application() :
    bus_connection_(nullptr),
    manager_(nullptr),
    input_source_(0),
    object_manager_(nullptr) {

    rl_callback_handler_install("aethercastctl> ", &Application::OnReadlineMessage);

    main_loop_ = g_main_loop_new(nullptr, FALSE);

    GError *error = nullptr;
    bus_connection_ = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!bus_connection_) {
        std::cerr << "Failed to connect to system bus: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    g_bus_watch_name_on_connection(bus_connection_, "org.aethercast",
                                   G_BUS_NAME_WATCHER_FLAGS_NONE,
                                   &Application::OnServiceFound,
                                   &Application::OnServiceLost,
                                   this, nullptr);

    g_dbus_connection_signal_subscribe(bus_connection_, "org.aethercast", "org.freedesktop.DBus.Properties", "PropertiesChanged", "/org/aethercast", nullptr,
                                       G_DBUS_SIGNAL_FLAGS_NONE, &Application::OnManagerPropertiesChanged, this, nullptr);

    RegisterCommand(Command { "enable", "", "Enable manager", std::bind(&Application::HandleEnableCommand, this, _1) });
    RegisterCommand(Command { "disable", "", "Disable manager", std::bind(&Application::HandleDisableCommand, this, _1) });
    RegisterCommand(Command { "show", "", "Show manager properties", std::bind(&Application::HandleShowCommand, this, _1) });
    RegisterCommand(Command { "scan", "", "Scan for devices", std::bind(&Application::HandleScanCommand, this, _1) });
    RegisterCommand(Command { "devices", "", "List available devices", std::bind(&Application::HandleDevicesCommand, this, _1) });
    RegisterCommand(Command { "info", "<address>", "Show device information", std::bind(&Application::HandleInfoCommand, this, _1) });
    RegisterCommand(Command { "connect", "<address>", "Connect a device", std::bind(&Application::HandleConnectCommand, this, _1) });
    RegisterCommand(Command { "disconnect", "<address>", "Disconnect a device", std::bind(&Application::HandleDisconnectCommand, this, _1) });
}

Application::~Application() {
    rl_message();
    rl_callback_handler_remove();

    g_object_unref(bus_connection_);
    g_main_loop_unref(main_loop_);

    if (manager_)
        g_object_unref(manager_);

    if (input_source_ > 0)
        g_source_remove(input_source_);
}

int Application::Run() {
    g_main_loop_run(main_loop_);
    return 0;
}

void Application::RegisterCommand(const Command &command) {
    available_commands_[command.name] = command;
}

void Application::HandleEnableCommand(const std::string &arguments) {
    if (!manager_)
        return;

    aethercast_interface_manager_set_enabled(manager_, true);
}

void Application::HandleDisableCommand(const std::string &arguments) {
    if (!manager_)
        return;

    aethercast_interface_manager_set_enabled(manager_, false);
}

void Application::HandleShowCommand(const std::string &arguments) {
    if (!manager_)
        return;

    auto enabled = aethercast_interface_manager_get_enabled(manager_);
    std::cout << "Enabled: " << std::boolalpha << (bool) enabled << std::endl;

    auto state = aethercast_interface_manager_get_state(manager_);
    std::cout << "State: " << state << std::endl;

    auto scanning = aethercast_interface_manager_get_scanning(manager_);
    std::cout << "Scanning: " << std::boolalpha << (bool) scanning << std::endl;

    auto capabilities = aethercast_interface_manager_get_capabilities(manager_);
    std::cout << "Capabilities:" << std::endl;
    for (int n = 0; capabilities[n] != nullptr; n++)
        std::cout << "  " << capabilities[n] << std::endl;
}

void Application::OnScanDone(GObject *object, GAsyncResult *res, gpointer user_data) {
    auto inst = static_cast<Application*>(user_data);

    GError *error = nullptr;
    if (!aethercast_interface_manager_call_scan_finish(inst->manager_, res, &error)) {
        std::cerr << "Failed to scan:" << error->message << std::endl;
        g_error_free(error);
        return;
    }
}

void Application::HandleScanCommand(const std::string &arguments) {
    if (!manager_)
        return;

    aethercast_interface_manager_call_scan(manager_, nullptr, &Application::OnScanDone, this);
}

void Application::ForeachDevice(std::function<void(AethercastInterfaceDevice*)> callback, const std::string &address_filter) {
    if (!callback)
        return;

    auto objects = g_dbus_object_manager_get_objects(object_manager_);

    for (auto obj = objects; obj != nullptr; obj = obj->next) {
        auto device = AETHERCAST_INTERFACE_OBJECT_PROXY(obj->data);
        if (!device)
            continue;

        auto iface = g_dbus_object_get_interface(G_DBUS_OBJECT(device), "org.aethercast.Device");
        if (!iface)
            continue;

        auto device_obj = AETHERCAST_INTERFACE_DEVICE(iface);

        if (address_filter.length() > 0) {
            auto device_address = aethercast_interface_device_get_address(device_obj);
            if (address_filter != device_address)
                continue;
        }

        callback(device_obj);
    }
}

void Application::HandleDevicesCommand(const std::string &arguments) {
    ForeachDevice([=](AethercastInterfaceDevice *device) {
        auto address = aethercast_interface_device_get_address(device);
        auto name = aethercast_interface_device_get_name(device);
        std::cout << "Device " << address << " " << name << std::endl;
    });
}

void Application::HandleInfoCommand(const std::string &arguments) {
    if (arguments.length() == 0) {
        std::cerr << "No device address supplied" << std::endl;
        return;
    }

    bool found = false;

    ForeachDevice([&](AethercastInterfaceDevice *device) {
        auto address = aethercast_interface_device_get_address(device);
        std::cout << "Address: " << address << std::endl;

        auto name = aethercast_interface_device_get_name(device);
        std::cout << "Name: " << name << std::endl;

        auto state = aethercast_interface_device_get_state(device);
        std::cout << "State: " << state << std::endl;

        auto capabilities = aethercast_interface_device_get_capabilities(device);
        std::cout << "Capabilities: " << std::endl;
        for (int n = 0; capabilities[n] != nullptr; n++)
            std::cout << "  " << capabilities[n] << std::endl;

        found = true;
    }, arguments);

    if (!found)
        std::cerr << "Unknown or invalid device address" << std::endl;
}

void Application::OnDeviceConnected(GObject *object, GAsyncResult *res, gpointer user_data) {
    PromptSaver ps;

    GError *error = nullptr;
    if (!aethercast_interface_device_call_connect_finish(AETHERCAST_INTERFACE_DEVICE(object), res, &error)) {
        std::cerr << "Failed to connect with device: " << error->message << std::endl;
        g_error_free(error);
        return;
    }
}

void Application::HandleConnectCommand(const std::string &arguments) {
    PromptSaver ps;

    if (arguments.length() == 0) {
        std::cerr << "No device address supplied" << std::endl;
        return;
    }

    bool found = false;

    ForeachDevice([&](AethercastInterfaceDevice *device) {

        aethercast_interface_device_call_connect(device, "sink", nullptr,
                                               &Application::OnDeviceConnected, nullptr);

        found = true;
    }, arguments);

    if (!found)
        std::cerr << "Unknown or invalid device address" << std::endl;
}

void Application::OnDeviceDisconnected(GObject *object, GAsyncResult *res, gpointer user_data) {
    PromptSaver ps;

    GError *error = nullptr;
    if (!aethercast_interface_device_call_disconnect_finish(AETHERCAST_INTERFACE_DEVICE(object), res, &error)) {
        std::cerr << "Failed to disconnect from device: " << error->message << std::endl;
        g_error_free(error);
        return;
    }
}

void Application::HandleDisconnectCommand(const std::string &arguments) {
    PromptSaver ps;

    if (arguments.length() == 0) {
        std::cerr << "No device address supplied" << std::endl;
        return;
    }

    bool found = false;

    ForeachDevice([&](AethercastInterfaceDevice *device) {

        aethercast_interface_device_call_disconnect(device, nullptr,
                                               &Application::OnDeviceDisconnected, nullptr);

        found = true;
    }, arguments);

    if (!found)
        std::cerr << "Unknown or invalid device address" << std::endl;
}

void Application::SetupStandardInput() {
    GIOChannel *channel;

    channel = g_io_channel_unix_new(fileno(stdin));

    input_source_ = g_io_add_watch(channel,
                            (GIOCondition) (G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL),
                            &Application::OnUserInput, this);

    g_io_channel_unref(channel);

}

gboolean Application::OnUserInput(GIOChannel *channel, GIOCondition condition, gpointer user_data) {
    auto inst = static_cast<Application*>(user_data);

    if (condition & G_IO_IN) {
        rl_callback_read_char();
        return TRUE;
    }

    if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
        g_main_loop_quit(inst->main_loop_);
        return FALSE;
    }

    return TRUE;
}

void Application::OnReadlineMessage(char *input) {
    PromptSaver ps;

    char *arg;
    int i;

    if (!input) {
        rl_newline(1, '\n');
        g_main_loop_quit(main_loop_);
        return;
    }

    std::string cmd = strtok_r(input, " ", &arg) ? : "";
    if (cmd.size() == 0)
        return;

    if (arg) {
        int len = strlen(arg);
        if (len > 0 && arg[len - 1] == ' ')
            arg[len - 1] = '\0';
    }

    auto iter = available_commands_.find(cmd);
    if (iter != available_commands_.end()) {
        if (iter->second.func)
            iter->second.func(arg);
    }
    else if (cmd == "help") {
        std::cout << "Available commands:" << std::endl;

        for (auto cmd : available_commands_) {
            if (cmd.second.description.size() == 0)
                continue;

            fprintf(stdout, " %s %-*s %s\n", cmd.second.name.c_str(),
                   (int)(25 - cmd.second.name.length()),
                   cmd.second.arguments.c_str(),
                   cmd.second.description.c_str());
        }
    }
    else {
        std::cerr << "Invalid command" << std::endl;
    }

    g_free(input);
}

void Application::OnServiceLost(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    auto inst = static_cast<Application*>(user_data);

    if (inst->input_source_ > 0) {
        g_source_remove(inst->input_source_);
        inst->input_source_ = 0;
    }

    if (inst->manager_) {
        g_object_unref(inst->manager_);
        inst->manager_ = nullptr;
    }

    if (inst->object_manager_) {
        g_object_unref(inst->object_manager_);
        inst->object_manager_ = nullptr;
    }
}

void Application::OnServiceFound(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data) {
    auto inst = static_cast<Application*>(user_data);

    aethercast_interface_manager_proxy_new(inst->bus_connection_, G_DBUS_PROXY_FLAGS_NONE,
                                         "org.aethercast", "/org/aethercast", nullptr,
                                         &Application::OnManagerConnected, inst);
}

void Application::OnManagerConnected(GObject *object, GAsyncResult *res, gpointer user_data) {
    PromptSaver ps;

    auto inst = static_cast<Application*>(user_data);

    GError *error = nullptr;
    inst->manager_ = aethercast_interface_manager_proxy_new_finish(res, &error);
    if (!inst->manager_) {
        std::cerr << "Could not connect with manager: " << error->message << std::endl;
        g_error_free(error);
        g_main_loop_quit(inst->main_loop_);
        return;
    }

    // Use a high enough timeout to make sure we wait enough for the service to
    // respond as the connection process can take some time depending on different
    // factors.
    g_dbus_proxy_set_default_timeout(G_DBUS_PROXY(inst->manager_), kDBusTimeout * 1000);

    aethercast_interface_object_manager_client_new(inst->bus_connection_,
                             G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                             "org.aethercast", "/org/aethercast",
                             nullptr, &Application::OnObjectManagerCreated, inst);
}

void Application::OnManagerPropertiesChanged(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
                                             const gchar *interface_name, const gchar *signal_name, GVariant *parameters,
                                             gpointer user_data) {
    PromptSaver ps;

    if (g_variant_n_children(parameters) != 3)
        return;

    GVariant *changed_props = g_variant_get_child_value(parameters, 1);
    for (int n = 0; n < g_variant_n_children(changed_props); n++) {
        auto element = g_variant_get_child_value(changed_props, n);
        auto key_v = g_variant_get_child_value(element, 0);
        auto value_v = g_variant_get_child_value(element, 1);

        auto key = std::string(g_variant_get_string(key_v, nullptr));

        std::cout << "[CHG] Manager " << key << " changed: ";

        if (key == "Enabled")
            std::cout << std::boolalpha << (bool) g_variant_get_boolean(g_variant_get_variant(value_v)) << std::endl;
        else if (key == "State")
            std::cout << g_variant_get_string(g_variant_get_variant(value_v), nullptr) << std::endl;
        else if (key == "Scanning")
            std::cout << std::boolalpha << (bool) g_variant_get_boolean(g_variant_get_variant(value_v)) << std::endl;
        else if (key == "Capabilities") {
            std::stringstream capabilities;

            for (int m = 0; m < g_variant_n_children(value_v); m++) {
                auto capability = g_variant_get_child_value(value_v, m);
                if (!g_variant_is_of_type(capability, G_VARIANT_TYPE_STRING))
                    continue;

                capabilities << g_variant_get_string(capability, nullptr) << " ";
            }

            std::cout << capabilities.str() << std::endl;
        }
        else
            std::cout << "unknown" << std::endl;

        g_variant_unref(key_v);
        g_variant_unref(value_v);
    }
}

void Application::OnDeviceAdded(GDBusObjectManager *manager, GDBusObject *object, gpointer user_data) {
    PromptSaver ps;

    auto iface = g_dbus_object_get_interface(G_DBUS_OBJECT(object), "org.aethercast.Device");
    if (!iface)
        return;

    auto device = AETHERCAST_INTERFACE_DEVICE(iface);

    auto address = aethercast_interface_device_get_address(device);
    auto name = aethercast_interface_device_get_name(device);

    std::cout << "Device " << address << " " << name << " added" << std::endl;
}

void Application::OnDeviceRemoved(GDBusObjectManager *manager, GDBusObject *object, gpointer user_data) {
    PromptSaver ps;

    auto iface = g_dbus_object_get_interface(G_DBUS_OBJECT(object), "org.aethercast.Device");
    if (!iface)
        return;

    auto device = AETHERCAST_INTERFACE_DEVICE(iface);

    auto address = aethercast_interface_device_get_address(device);
    auto name = aethercast_interface_device_get_name(device);

    std::cout << "Device " << address << " " << name <<" removed" << std::endl;
}

void Application::OnObjectManagerCreated(GObject *object, GAsyncResult *res, gpointer user_data) {
    auto inst = static_cast<Application*>(user_data);

    GError *error = nullptr;
    inst->object_manager_ = aethercast_interface_object_manager_client_new_finish(res, &error);
    if (!inst->object_manager_) {
        g_error_free(error);
        return;
    }

    g_signal_connect(inst->object_manager_, "object-added", G_CALLBACK(&Application::OnDeviceAdded), inst);
    g_signal_connect(inst->object_manager_, "object-removed", G_CALLBACK(&Application::OnDeviceRemoved), inst);

    inst->SetupStandardInput();
}

} // namespace client
} // namespace ac
