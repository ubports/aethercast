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

#include <glib.h>

#include "miracastserviceadapter.h"

namespace {
// KeepAlive helps in delivering an object securely through a callback-based
// system that takes context as a void* (e.g., the glib calls in this class).
// A KeepAlive<T> instance simply wraps a managed ptr to an instance of T and keeps
// it alive while an async operation is in progress. ShouldDie() is just a concise way of
// unwrapping the instance and cleaning up the KeepAlive<T> instance itself.
template<typename T>
class KeepAlive {
public:
    KeepAlive(const std::shared_ptr<T>& inst) : inst_(inst) {
    }

    std::shared_ptr<T> ShouldDie() {
        auto inst = inst_;
        delete this;
        return inst;
    }

private:
    std::shared_ptr<T> inst_;
};
}

namespace mcs {
std::shared_ptr<MiracastServiceAdapter> MiracastServiceAdapter::create(const std::shared_ptr<MiracastService> &service) {
    return std::shared_ptr<MiracastServiceAdapter>(new MiracastServiceAdapter(service))->FinalizeConstruction();
}

MiracastServiceAdapter::MiracastServiceAdapter(const std::shared_ptr<MiracastService> &service) :
    service_(service),
    manager_obj_(nullptr),
    bus_id_(0),
    object_manager_(nullptr) {
}

MiracastServiceAdapter::~MiracastServiceAdapter() {
    if (bus_id_ > 0)
        g_bus_unown_name(bus_id_);

    // We do not have to disconnect our handlers from:
    //   - handle-scan
    //   - handle-connect-sink
    // as we own the object emitting those signals.
}

void MiracastServiceAdapter::OnStateChanged(NetworkDeviceState state) {
}

void MiracastServiceAdapter::OnDeviceFound(const NetworkDevice::Ptr &peer) {
}

void MiracastServiceAdapter::OnDeviceLost(const NetworkDevice::Ptr &peer) {
}

void MiracastServiceAdapter::OnNameAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    auto inst = static_cast<KeepAlive<MiracastServiceAdapter>*>(user_data)->ShouldDie();
    inst->manager_obj_.reset(miracast_interface_manager_skeleton_new());

    g_signal_connect(inst->manager_obj_.get(), "handle-scan",
                     G_CALLBACK(&MiracastServiceAdapter::OnHandleScan), inst.get());
    g_signal_connect(inst->manager_obj_.get(), "handle-connect-sink",
                     G_CALLBACK(&MiracastServiceAdapter::OnHandleConnectSink), inst.get());

    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(inst->manager_obj_.get()),
                                     connection, kManagerPath, nullptr);


    inst->object_manager_.reset(g_dbus_object_manager_server_new(kManagerPath));
    g_dbus_object_manager_server_set_connection(inst->object_manager_.get(), connection);

    g_message("Registered bus name %s", name);
}

void MiracastServiceAdapter::OnHandleScan(MiracastInterfaceManager *skeleton,
                                        GDBusMethodInvocation *invocation, gpointer user_data) {
    auto inst = static_cast<MiracastServiceAdapter*>(user_data);
    g_message("Scanning for remote devices");

    inst->service_->Scan();

    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void MiracastServiceAdapter::OnHandleConnectSink(MiracastInterfaceManager *skeleton,
                                        GDBusMethodInvocation *invocation, const gchar *address, gpointer user_data) {
    auto inst = static_cast<MiracastServiceAdapter*>(user_data);
    inst->service_->ConnectSink(mcs::MacAddress(address), [=](bool success, const std::string &error_text) {
        if (!success) {
            g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                                                  "%s", error_text.c_str());
            return;
        }

        g_dbus_method_invocation_return_value(invocation, nullptr);
    });

}

std::shared_ptr<MiracastServiceAdapter> MiracastServiceAdapter::FinalizeConstruction() {
    auto sp = shared_from_this();

    g_message("Created miracast service adapter");

    bus_id_ = g_bus_own_name(G_BUS_TYPE_SYSTEM, kBusName, G_BUS_NAME_OWNER_FLAGS_NONE,
                   nullptr, &MiracastServiceAdapter::OnNameAcquired, nullptr, new KeepAlive<MiracastServiceAdapter>{sp}, nullptr);
    if (bus_id_ == 0) {
        g_warning("Failed to register bus name 'com.canonical.miracast'");
    }

    service_->SetDelegate(sp);
    return sp;
}
} // namespace mcs
