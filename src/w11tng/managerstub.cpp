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

#include <ac/logger.h>
#include <ac/keep_alive.h>

#include "managerstub.h"

namespace w11tng {

ManagerStub::Ptr ManagerStub::Create() {
    return std::shared_ptr<ManagerStub>(new ManagerStub)->FinalizeConstruction();
}

ManagerStub::Ptr ManagerStub::FinalizeConstruction() {
    auto sp = shared_from_this();

    GError *error = nullptr;
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!connection_) {
        AC_ERROR("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    wpa_supplicant_fi_w1_wpa_supplicant1_proxy_new(connection_.get(),
                                                   G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                   kBusName,
                                                   kManagerPath,
                                                   nullptr,
                                                   [](GObject *source, GAsyncResult *res, gpointer user_data) {

        boost::ignore_unused_variable_warning(source);

        auto inst = static_cast<ac::SharedKeepAlive<ManagerStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        inst->proxy_.reset(wpa_supplicant_fi_w1_wpa_supplicant1_proxy_new_finish(res, &error));
        if (!inst->proxy_) {
            AC_ERROR("Failed to connect with supplicant manager object: %s", error->message);
            g_error_free(error);
            return;
        }

        inst->Initialize();

        if (auto sp = inst->delegate_.lock())
            sp->OnManagerReady();

    }, new ac::SharedKeepAlive<ManagerStub>{sp});
    return sp;
}

ManagerStub::ManagerStub() :
    p2p_supported_(false) {
}

ManagerStub::~ManagerStub() {
}

void ManagerStub::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

void ManagerStub::ResetDelegate() {
    delegate_.reset();
}

void ManagerStub::OnInterfaceAdded(GObject *source, const gchar *path, GVariant *properties, gpointer user_data) {
    auto inst = static_cast<ac::WeakKeepAlive<ManagerStub>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    AC_DEBUG("path %s", path);

    inst->interfaces_.push_back(path);

    if (auto sp = inst->delegate_.lock())
        sp->OnManagerInterfaceAdded(std::string(path));
}

void ManagerStub::OnInterfaceRemoved(GObject *source, const gchar *path, gpointer user_data) {
    auto inst = static_cast<ac::WeakKeepAlive<ManagerStub>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    AC_DEBUG("path %s", path);

    inst->interfaces_.erase(std::remove(inst->interfaces_.begin(), inst->interfaces_.end(), std::string(path)),
                            inst->interfaces_.end());

    if (auto sp = inst->delegate_.lock())
        sp->OnManagerInterfaceRemoved(std::string(path));
}

void ManagerStub::Initialize() {
    auto capabilities = wpa_supplicant_fi_w1_wpa_supplicant1_get_capabilities(proxy_.get());

    g_signal_connect_data(proxy_.get(), "interface-added",
                          G_CALLBACK(&ManagerStub::OnInterfaceAdded),
                          new ac::WeakKeepAlive<ManagerStub>(shared_from_this()),
                          [](gpointer data, GClosure *) { delete static_cast<ac::WeakKeepAlive<ManagerStub>*>(data); },
                          GConnectFlags(0));

    g_signal_connect_data(proxy_.get(), "interface-removed",
                          G_CALLBACK(&ManagerStub::OnInterfaceRemoved),
                          new ac::WeakKeepAlive<ManagerStub>(shared_from_this()),
                          [](gpointer data, GClosure *) { delete static_cast<ac::WeakKeepAlive<ManagerStub>*>(data); },
                          GConnectFlags(0));

    if (!capabilities) {
        AC_WARNING("Could not retrieve any capabilities from supplicant. Aborting.");
        return;
    }

    int n = 0;
    for (auto capability = capabilities[n]; capability != nullptr; n++, capability = capabilities[n]) {
        if (std::string(capability) == "p2p")
            p2p_supported_ = true;

        capabilities_.push_back(capability);
    }

    if (!p2p_supported_)
        return;

    auto interfaces = wpa_supplicant_fi_w1_wpa_supplicant1_get_interfaces(proxy_.get());
    if (!interfaces) {
        AC_WARNING("No WiFi interface available. Waiting for one to appear.");
        return;
    }

    interfaces_.clear();
    n = 0;
    for (auto iface = interfaces[n]; iface != nullptr; n++, iface = interfaces[n])
        interfaces_.push_back(iface);
}

void ManagerStub::SetWFDIEs(uint8_t *bytes, int length) {
    auto ie_value = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, bytes, length, 1);
    wpa_supplicant_fi_w1_wpa_supplicant1_set_wfdies(proxy_.get(), ie_value);
}

void ManagerStub::CreateInterface(const std::string &interface_name) {
    AC_DEBUG("interface %s", interface_name);

    auto builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(builder, "{sv}", "Ifname", g_variant_new_string(interface_name.c_str()));
    auto args = g_variant_builder_end(builder);

    wpa_supplicant_fi_w1_wpa_supplicant1_call_create_interface(proxy_.get(), args, nullptr,
                                     [](GObject *source, GAsyncResult *res, gpointer user_data) {

        boost::ignore_unused_variable_warning(source);

        auto inst = static_cast<ac::SharedKeepAlive<ManagerStub>*>(user_data)->ShouldDie();

        gchar *path = nullptr;
        GError *error = nullptr;
        if (!wpa_supplicant_fi_w1_wpa_supplicant1_call_create_interface_finish(inst->proxy_.get(), &path, res, &error)) {
            AC_ERROR("Failed to create interface: %s", error->message);
            g_error_free(error);

            if (auto sp = inst->delegate_.lock())
                sp->OnManagerInterfaceCreationFailed();

            return;
        }
    }, new ac::SharedKeepAlive<ManagerStub>{shared_from_this()});
}

bool ManagerStub::IsP2PSupported() const {
    return p2p_supported_;
}

std::vector<std::string> ManagerStub::Capabilities() const {
    return capabilities_;
}

std::vector<std::string> ManagerStub::Interfaces() const {
    return interfaces_;
}

} // namespace w11tng
