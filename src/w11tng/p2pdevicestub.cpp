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

#include <boost/concept_check.hpp>

#include <mcs/logger.h>
#include <mcs/keep_alive.h>
#include <mcs/dbushelpers.h>

#include "p2pdevicestub.h"

namespace w11tng {

P2PDeviceStub::Ptr P2PDeviceStub::Create(const std::weak_ptr<P2PDeviceStub::Delegate> &delegate) {
    return std::shared_ptr<P2PDeviceStub>(new P2PDeviceStub(delegate))->FinalizeConstruction();
}

std::shared_ptr<P2PDeviceStub> P2PDeviceStub::FinalizeConstruction() {
    auto sp = shared_from_this();

    GError *error = nullptr;
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!connection_) {
        MCS_ERROR("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    g_bus_watch_name_on_connection(connection_.get(),
                                   kBusName,
                                   G_BUS_NAME_WATCHER_FLAGS_NONE,
                                   &P2PDeviceStub::OnServiceFound,
                                   &P2PDeviceStub::OnServiceLost,
                                   new mcs::WeakKeepAlive<P2PDeviceStub>(sp),
                                   nullptr);

    return sp;
}

P2PDeviceStub::P2PDeviceStub(const std::weak_ptr<P2PDeviceStub::Delegate> &delegate) :
    delegate_(delegate),
    p2p_supported_(false),
    scan_timeout_source_(0) {
}

P2PDeviceStub::~P2PDeviceStub() {
}

void P2PDeviceStub::OnServiceFound(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data) {
    boost::ignore_unused_variable_warning(connection);
    boost::ignore_unused_variable_warning(name);
    boost::ignore_unused_variable_warning(name_owner);

    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    wpa_supplicant_fi_w1_wpa_supplicant1_proxy_new(inst->connection_.get(),
                                                   G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                   kBusName,
                                                   kManagerPath,
                                                   nullptr,
                                                   [](GObject *source, GAsyncResult *res, gpointer user_data) {

        boost::ignore_unused_variable_warning(source);

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        inst->manager_.reset(wpa_supplicant_fi_w1_wpa_supplicant1_proxy_new_finish(res, &error));
        if (!inst->manager_) {
            MCS_ERROR("Failed to connect with supplicant manager object: %s", error->message);
            g_error_free(error);
            return;
        }

        inst->Initialize();

    }, new mcs::SharedKeepAlive<P2PDeviceStub>{inst->shared_from_this()});
}

void P2PDeviceStub::OnServiceLost(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    boost::ignore_unused_variable_warning(connection);
    boost::ignore_unused_variable_warning(name);

    MCS_DEBUG("Lost supplicant service");

    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    inst->Reset();

    if (auto sp = inst->delegate_.lock())
        sp->OnSystemKilled();
}

void P2PDeviceStub::Initialize() {
    auto capabilities = wpa_supplicant_fi_w1_wpa_supplicant1_get_capabilities(manager_.get());

    if (!capabilities) {
        MCS_WARNING("Could not retrieve any capabilities from supplicant. Aborting.");
        return;
    }

    int n = 0;
    for (auto capability = capabilities[n]; capability != nullptr; n++, capability = capabilities[n]) {
        if (std::string(capability) == "p2p") {
            p2p_supported_ = true;
            break;
        }
    }

    if (!p2p_supported_)
        return;

    MCS_DEBUG("System supports P2P");

    auto interfaces = wpa_supplicant_fi_w1_wpa_supplicant1_get_interfaces(manager_.get());
    if (!interfaces) {
        MCS_WARNING("No WiFi interface available. Waiting for one to appear.");
        return;
    }

    n = 0;
    for (auto iface = interfaces[n]; iface != nullptr; n++, iface = interfaces[n]) {
        wpa_supplicant_interface_proxy_new(connection_.get(),
                                           G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                           kBusName,
                                           iface,
                                           nullptr, [](GObject *source, GAsyncResult *res, gpointer user_data) {

                auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

                GError *error = nullptr;
                auto proxy = wpa_supplicant_interface_proxy_new_finish(res, &error);
                if (!proxy) {
                    MCS_ERROR("Failed to create proxy for interface: %s", error->message);
                    g_error_free(error);
                    return;
                }

                bool supports_p2p = false;

                auto capabilities = wpa_supplicant_interface_get_capabilities(proxy);
                mcs::DBusHelpers::ParseDictionary(capabilities, [&](const std::string &key, GVariant *value) {
                    mcs::DBusHelpers::ParseArray(g_variant_get_variant(value), [&](GVariant *mode) {
                        if (std::string(g_variant_get_string(mode, nullptr)) == "p2p")
                            supports_p2p = true;
                    });
                }, "Modes");

                if (supports_p2p) {
                    MCS_DEBUG("Found interface which supports P2P");
                    // We take the first interface which supports p2p here and ignore
                    // all others. That is really enough for now.
                    inst->iface_.reset(proxy);
                    inst->InitializeP2P();
                    return;
                }

                g_object_unref(proxy);

        }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});
    }
}

void P2PDeviceStub::OnFindStopped(gpointer user_data) {
}

void P2PDeviceStub::OnDeviceFound(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, gpointer user_data) {
    MCS_DEBUG("Found device %s", path);

    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    if (auto sp = inst->delegate_.lock())
        sp->OnDeviceFound(std::string(path));
}

void P2PDeviceStub::OnDeviceLost(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, gpointer user_data) {
    MCS_DEBUG("Lost device %s", path);

    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    if (auto sp = inst->delegate_.lock())
        sp->OnDeviceLost(std::string(path));
}

void P2PDeviceStub::InitializeP2P() {

    auto object_path = g_dbus_proxy_get_object_path(G_DBUS_PROXY(iface_.get()));

    wpa_supplicant_interface_p2_pdevice_proxy_new(connection_.get(),
                                                  G_DBUS_PROXY_FLAGS_NONE,
                                                  kBusName,
                                                  object_path,
                                                  nullptr,
                                                  [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        inst->p2p_device_.reset(wpa_supplicant_interface_p2_pdevice_proxy_new_finish(res, &error));
        if (!inst->p2p_device_) {
            MCS_ERROR("Failed to connect with P2P interface: %s", error->message);
            g_error_free(error);
            return;
        }

        inst->ConnectSignals();

        MCS_DEBUG("Successfully setup P2P device proxy");

        if (auto sp = inst->delegate_.lock())
            sp->OnChanged();

    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});
}

void P2PDeviceStub::ConnectSignals() {
    auto sp = shared_from_this();

    // FIXME the FindStopped signal is supported with supplicant >= 2.5 but we add it here to
    // be ready for the future.
    g_signal_connect_data(p2p_device_.get(), "find-stopped",
                          G_CALLBACK(&P2PDeviceStub::OnFindStopped), new mcs::WeakKeepAlive<P2PDeviceStub>(sp),
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(data); },
                          GConnectFlags(0));

    g_signal_connect_data(p2p_device_.get(), "device-found",
                          G_CALLBACK(&P2PDeviceStub::OnDeviceFound), new mcs::WeakKeepAlive<P2PDeviceStub>(sp),
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(data); },
                          GConnectFlags(0));

    g_signal_connect_data(p2p_device_.get(), "device-lost",
                          G_CALLBACK(&P2PDeviceStub::OnDeviceLost), new mcs::WeakKeepAlive<P2PDeviceStub>(sp),
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(data); },
                          GConnectFlags(0));

}

void P2PDeviceStub::Reset() {
    p2p_supported_ = false;
    manager_.reset();
    iface_.reset();
}

bool P2PDeviceStub::IsP2PSupport() const {
    return p2p_supported_;
}

void P2PDeviceStub::StartFindTimeout() {
    scan_timeout_source_ = g_timeout_add_seconds(scan_timeout_.count(), [](gpointer user_data) {
        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        inst->scan_timeout_source_ = 0;
        inst->scan_timeout_ = std::chrono::seconds{0};
        inst->StopFind();

        if (auto sp = inst->delegate_.lock())
            sp->OnChanged();

        return FALSE;
    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});
}

void P2PDeviceStub::Find(const std::chrono::seconds &timeout) {
    if (!p2p_device_ || scan_timeout_source_ > 0)
        return;

    MCS_DEBUG("timeout %d", timeout.count());

    scan_timeout_ = timeout;

    // FIXME wpa-supplicant starts to implement the FindStopped signal starting
    // with 2.5 so we have to emulate the scan timeout until we can switch to it:
    // auto builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
    // g_variant_builder_add(builder, "{sv}", "Timeout", g_variant_new_int32(timeout.count()));
    // auto arguments = g_variant_builder_end(builder);

    // glib is rather unhappy if we just pass a nullptr for the arguments if we
    // don't use them so lets create just an empty array
    auto arguments = g_variant_new_array(g_variant_type_new("{sv}"), nullptr, 0);

    wpa_supplicant_interface_p2_pdevice_call_find(p2p_device_.get(), arguments, nullptr,
                                                  [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        if (!wpa_supplicant_interface_p2_pdevice_call_find_finish(inst->p2p_device_.get(), res, &error)) {
            MCS_ERROR("Failed to start P2P discovery: %s", error->message);
            g_error_free(error);
            return;
        }

        inst->StartFindTimeout();

        if (auto sp = inst->delegate_.lock())
            sp->OnChanged();

    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});
}

void P2PDeviceStub::StopFind() {
    if (!p2p_device_)
        return;

    MCS_DEBUG("");

    wpa_supplicant_interface_p2_pdevice_call_stop_find(p2p_device_.get(), nullptr,
                                                  [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        if (!wpa_supplicant_interface_p2_pdevice_call_stop_find_finish(inst->p2p_device_.get(), res, &error)) {
            MCS_ERROR("Failed to stop P2P discovery: %s", error->message);
            g_error_free(error);
            return;
        }

    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});
}

bool P2PDeviceStub::Connect(const w11tng::NetworkDevice::Ptr &device) {
    if (!p2p_device_ || !device)
        return false;

    MCS_DEBUG("address %s", device->Address());

    auto builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);

    g_variant_builder_add(builder, "{sv}", "peer", g_variant_new_string(device->ObjectPath().c_str()));

    auto arguments = g_variant_builder_end(builder);

    wpa_supplicant_interface_p2_pdevice_call_connect(p2p_device_.get(), arguments, nullptr,
                                                     [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        if (!wpa_supplicant_interface_p2_pdevice_call_connect_finish(inst->p2p_device_.get(), res, &error)) {
            MCS_ERROR("Failed to connect with P2P device: %s", error->message);
            g_error_free(error);
            return;
        }

    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});

    return true;
}

bool P2PDeviceStub::Disconnect() {
    MCS_DEBUG("");

    return false;
}

} // namespace w11tng
