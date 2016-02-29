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
#include <gio/gio.h>

#include <algorithm>

#include <boost/concept_check.hpp>

#include "miracastcontrollerskeleton.h"
#include "keep_alive.h"
#include "utils.h"
#include "dbushelpers.h"
#include "logger.h"

namespace mcs {
std::shared_ptr<MiracastControllerSkeleton> MiracastControllerSkeleton::create(const std::shared_ptr<MiracastController> &controller) {
    return std::shared_ptr<MiracastControllerSkeleton>(new MiracastControllerSkeleton(controller))->FinalizeConstruction();
}

MiracastControllerSkeleton::MiracastControllerSkeleton(const std::shared_ptr<MiracastController> &controller) :
    ForwardingMiracastController(controller),
    manager_obj_(nullptr),
    bus_connection_(nullptr),
    bus_id_(0),
    object_manager_(nullptr) {
}

MiracastControllerSkeleton::~MiracastControllerSkeleton() {
    if (bus_id_ > 0)
        g_bus_unown_name(bus_id_);

    // We do not have to disconnect our handlers from:
    //   - handle-scan
    //   - handle-connect-sink
    // as we own the object emitting those signals.
}

void MiracastControllerSkeleton::SyncProperties() {
    aethercast_interface_manager_set_state(manager_obj_.get(),
                                           NetworkDevice::StateToStr(State()).c_str());

    auto capabilities = DBusHelpers::GenerateCapabilities(Capabilities());

    aethercast_interface_manager_set_capabilities(manager_obj_.get(), capabilities);

    g_strfreev(capabilities);

    aethercast_interface_manager_set_scanning(manager_obj_.get(), Scanning());
}

void MiracastControllerSkeleton::OnStateChanged(NetworkDeviceState state) {
    if (!manager_obj_)
        return;

    aethercast_interface_manager_set_state(manager_obj_.get(),
                                           NetworkDevice::StateToStr(State()).c_str());
}

std::string MiracastControllerSkeleton::GenerateDevicePath(const NetworkDevice::Ptr &device) const {
    std::string address = device->Address();
    std::replace(address.begin(), address.end(), ':', '_');
    // FIXME using kManagerPath doesn't seem to work. Fails at link time ...
    return mcs::Utils::Sprintf("/org/aethercast/dev_%s", address.c_str());
}

void MiracastControllerSkeleton::OnDeviceFound(const NetworkDevice::Ptr &device) {
    DEBUG("device %s", device->Address().c_str());

    auto path = GenerateDevicePath(device);
    auto adapter = NetworkDeviceSkeleton::Create(bus_connection_, path , device, shared_from_this());
    devices_.insert(std::pair<std::string,NetworkDeviceSkeleton::Ptr>(device->Address(), adapter));

    g_dbus_object_manager_server_export(object_manager_.get(), adapter->DBusObject());
}

void MiracastControllerSkeleton::OnDeviceLost(const NetworkDevice::Ptr &device) {
    auto iter = devices_.find(device->Address());
    if (iter == devices_.end())
        return;

    g_dbus_object_manager_server_unexport(object_manager_.get(), iter->second->Path().c_str());

    devices_.erase(iter);
}

void MiracastControllerSkeleton::OnDeviceChanged(const NetworkDevice::Ptr &peer) {
    auto iter = devices_.find(peer->Address());
    if (iter == devices_.end())
        return;

    iter->second->SyncProperties();
}

void MiracastControllerSkeleton::OnChanged() {
    SyncProperties();
}

void MiracastControllerSkeleton::OnNameAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    auto inst = static_cast<SharedKeepAlive<MiracastControllerSkeleton>*>(user_data)->ShouldDie();
    inst->manager_obj_.reset(aethercast_interface_manager_skeleton_new());

    g_signal_connect_data(inst->manager_obj_.get(), "handle-scan",
                     G_CALLBACK(&MiracastControllerSkeleton::OnHandleScan), new WeakKeepAlive<MiracastControllerSkeleton>(inst),
                     [](gpointer data, GClosure *) { delete static_cast<WeakKeepAlive<MiracastControllerSkeleton>*>(data); }, GConnectFlags(0));

    g_signal_connect_data(inst->manager_obj_.get(), "handle-disconnect-all",
                     G_CALLBACK(&MiracastControllerSkeleton::OnHandleDisconnectAll), new WeakKeepAlive<MiracastControllerSkeleton>(inst),
                     [](gpointer data, GClosure *) { delete static_cast<WeakKeepAlive<MiracastControllerSkeleton>*>(data); }, GConnectFlags(0));

    inst->SyncProperties();

    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(inst->manager_obj_.get()),
                                     connection, kManagerPath, nullptr);

    inst->object_manager_.reset(g_dbus_object_manager_server_new(kManagerPath));
    g_dbus_object_manager_server_set_connection(inst->object_manager_.get(), connection);

    INFO("Registered bus name %s", name);
}

gboolean MiracastControllerSkeleton::OnHandleScan(AethercastInterfaceManager *skeleton,
                                              GDBusMethodInvocation *invocation, gpointer user_data) {
    boost::ignore_unused_variable_warning(skeleton);
    auto inst = static_cast<WeakKeepAlive<MiracastControllerSkeleton>*>(user_data)->GetInstance().lock();

    if (not inst) {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Invalid state");
        return TRUE;
    }

    INFO("Scanning for remote devices");

    auto error = inst->Scan();
    if (error != mcs::Error::kNone) {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "%s", mcs::ErrorToString(error).c_str());
        return TRUE;
    }

    g_dbus_method_invocation_return_value(invocation, nullptr);

    return TRUE;
}

gboolean MiracastControllerSkeleton::OnHandleDisconnectAll(AethercastInterfaceManager *skeleton,
                                                           GDBusMethodInvocation *invocation, gpointer user_data) {
    boost::ignore_unused_variable_warning(skeleton);
    auto inst = static_cast<WeakKeepAlive<MiracastControllerSkeleton>*>(user_data)->GetInstance().lock();

    if (not inst) {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Invalid state");
        return TRUE;
    }

    g_object_ref(invocation);
    auto inv = make_shared_gobject(invocation);

    inst->DisconnectAll([inv](mcs::Error error) {
        if (error != Error::kNone) {
            g_dbus_method_invocation_return_error(inv.get(), G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                                                  "%s", mcs::ErrorToString(error).c_str());
            return;
        }

        g_dbus_method_invocation_return_value(inv.get(), nullptr);
    });

    return TRUE;
}

std::shared_ptr<MiracastControllerSkeleton> MiracastControllerSkeleton::FinalizeConstruction() {
    auto sp = shared_from_this();

    GError *error = nullptr;
    bus_connection_ = make_shared_gobject(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!bus_connection_) {
        ERROR("Failed to connect with system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    bus_id_ = g_bus_own_name(G_BUS_TYPE_SYSTEM, kBusName, G_BUS_NAME_OWNER_FLAGS_NONE,
                   nullptr, &MiracastControllerSkeleton::OnNameAcquired, nullptr, new SharedKeepAlive<MiracastControllerSkeleton>{sp}, nullptr);
    if (bus_id_ == 0)
        WARNING("Failed to register bus name");

    SetDelegate(sp);
    return sp;
}
} // namespace mcs
