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

#include <mcs/logger.h>
#include <mcs/utils.h>
#include <mcs/keep_alive.h>

#include <w11tng/config.h>

#include "dhcplistenerstub.h"

namespace w11tng {

DhcpListenerStub::Ptr DhcpListenerStub::Create(const std::string &address) {
    return std::shared_ptr<DhcpListenerStub>(new DhcpListenerStub(address))->FinalizeConstruction();
}

DhcpListenerStub::DhcpListenerStub(const std::string &address) :
    address_(address) {
}

DhcpListenerStub::~DhcpListenerStub() {
}

DhcpListenerStub::Ptr DhcpListenerStub::FinalizeConstruction() {
    auto sp = shared_from_this();

    MCS_DEBUG("address %s", address_);

    GError *error = nullptr;
    connection_.reset(g_dbus_connection_new_for_address_sync(address_.c_str(), G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                                             nullptr, nullptr, &error));
    if (!connection_) {
        MCS_WARNING("Failed to connect to remote DBus server: %s", error->message);
        g_error_free(error);
        return sp;
    }

    return sp;
}

void DhcpListenerStub::EmitEventSync(const std::map<std::string, std::string> &properties) {
    if (!connection_)
        return;

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
    for (auto prop :  properties)
        g_variant_builder_add(&builder, "{sv}", prop.first.c_str(), g_variant_new_string(prop.second.c_str()));
    auto value = g_variant_new("(a{sv})", &builder);

    GError *error = nullptr;
    if (!g_dbus_connection_emit_signal(connection_.get(), nullptr, "/",
                                       "org.aethercast.dhcp_client", "Event", value, &error)) {
        MCS_ERROR("Could not send event signal: %s", error->message);
        g_error_free(error);
        return;
    }

    if (!g_dbus_connection_flush_sync(connection_.get(), nullptr, &error)) {
        MCS_ERROR("Could not flush connection: %s", error->message);
        g_error_free(error);
        return;
    }
}

std::string DhcpListenerStub::Address() const {
    return address_;
}

} // namespace w11tng
