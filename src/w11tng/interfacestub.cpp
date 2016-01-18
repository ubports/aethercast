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

#include <mcs/logger.h>
#include <mcs/keep_alive.h>

#include "interfacestub.h"

namespace w11tng {

InterfaceStub::Ptr InterfaceStub::Create(const std::string &object_path) {
    return std::shared_ptr<InterfaceStub>(new InterfaceStub)->FinalizeConstruction(object_path);
}

InterfaceStub::Ptr InterfaceStub::FinalizeConstruction(const std::string &object_path) {
    auto sp = shared_from_this();

    GError *error = nullptr;
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!connection_) {
        MCS_ERROR("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    wpa_supplicant_interface_proxy_new(connection_.get(),
                                       G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                       kBusName,
                                       object_path.c_str(),
                                       nullptr,
                                       [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<InterfaceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        inst->proxy_.reset(wpa_supplicant_interface_proxy_new_finish(res, &error));
        if (!inst->proxy_) {
            MCS_ERROR("Failed to connect with Interface proxy: %s", error->message);
            g_error_free(error);
            return;
        }

        if (auto sp = inst->delegate_.lock())
            sp->OnInterfaceReady();

    }, new mcs::SharedKeepAlive<InterfaceStub>{shared_from_this()});

    return sp;
}

InterfaceStub::InterfaceStub() {
}

InterfaceStub::~InterfaceStub() {
}

void InterfaceStub::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

void InterfaceStub::ResetDelegate() {
    delegate_.reset();
}

std::vector<std::string> InterfaceStub::Capabilities() const {
    std::vector<std::string> capabilities;
    return capabilities;
}
std::string InterfaceStub::Driver() const {
    return wpa_supplicant_interface_get_driver(proxy_.get());
}

std::string InterfaceStub::Ifname() const {
    return wpa_supplicant_interface_get_ifname(proxy_.get());
}

std::string InterfaceStub::ObjectPath() const {
    return g_dbus_proxy_get_object_path(G_DBUS_PROXY(proxy_.get()));
}

} // namespace w11tng
