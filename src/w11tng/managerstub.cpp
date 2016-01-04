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
        MCS_ERROR("Failed to connect to system bus: %s", error->message);
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

        auto inst = static_cast<mcs::SharedKeepAlive<ManagerStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        inst->proxy_.reset(wpa_supplicant_fi_w1_wpa_supplicant1_proxy_new_finish(res, &error));
        if (!inst->proxy_) {
            MCS_ERROR("Failed to connect with supplicant manager object: %s", error->message);
            g_error_free(error);
            return;
        }

        inst->Initialize();
        inst->ready_();

    }, new mcs::SharedKeepAlive<ManagerStub>{sp});
    return sp;
}

ManagerStub::ManagerStub() :
    p2p_supported_(false) {
}

ManagerStub::~ManagerStub() {
}

void ManagerStub::Initialize() {
    auto capabilities = wpa_supplicant_fi_w1_wpa_supplicant1_get_capabilities(proxy_.get());

    if (!capabilities) {
        MCS_WARNING("Could not retrieve any capabilities from supplicant. Aborting.");
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
        MCS_WARNING("No WiFi interface available. Waiting for one to appear.");
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
