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

#include <algorithm>
#include <boost/concept_check.hpp>

#include "miracastserviceadapter.h"
#include "networkdeviceadapter.h"
#include "utils.h"
#include "keep_alive.h"
#include "logger.h"
#include "dbushelpers.h"

namespace mcs {

NetworkDeviceAdapter::Ptr NetworkDeviceAdapter::Create(const SharedGObject<GDBusConnection> &connection, const std::string &path, const NetworkDevice::Ptr &device, const MiracastService::Ptr &service) {
    return std::shared_ptr<NetworkDeviceAdapter>(new NetworkDeviceAdapter(connection, path, device, service))->FinalizeConstruction();
}

NetworkDeviceAdapter::NetworkDeviceAdapter(const SharedGObject<GDBusConnection> &connection, const std::string &path, const NetworkDevice::Ptr &device, const MiracastService::Ptr &service) :
    connection_(connection),
    object_(nullptr),
    path_(path),
    device_(device),
    service_(service) {
}

NetworkDeviceAdapter::~NetworkDeviceAdapter() {
    if (device_iface_)
        g_object_unref(device_iface_);

    if (object_)
        g_object_unref(object_);
}

std::shared_ptr<NetworkDeviceAdapter> NetworkDeviceAdapter::FinalizeConstruction() {
    device_iface_ = aethercast_interface_device_skeleton_new();

    auto sp = shared_from_this();

    g_signal_connect(device_iface_, "handle-connect",
                     G_CALLBACK(&NetworkDeviceAdapter::OnHandleConnect),
                     new WeakKeepAlive<NetworkDeviceAdapter>{sp});
    g_signal_connect(device_iface_, "handle-disconnect",
                     G_CALLBACK(&NetworkDeviceAdapter::OnHandleDisconnect),
                     new WeakKeepAlive<NetworkDeviceAdapter>{sp});

    SyncProperties();

    object_ = aethercast_interface_object_skeleton_new(path_.c_str());
    if (!object_)
        ERROR("Failed to create object for device %s", device_->Address());
    else
        aethercast_interface_object_skeleton_set_device(object_, device_iface_);

    return sp;
}

void NetworkDeviceAdapter::SyncProperties() {
    aethercast_interface_device_set_address(device_iface_, device_->Address().c_str());
    aethercast_interface_device_set_name(device_iface_, device_->Name().c_str());
    aethercast_interface_device_set_state(device_iface_, NetworkDevice::StateToStr(device_->State()).c_str());

    auto capabilities = DBusHelpers::GenerateCapabilities(device_->SupportedRoles());
    aethercast_interface_device_set_capabilities(device_iface_, capabilities);
    g_strfreev(capabilities);
}

GDBusObjectSkeleton* NetworkDeviceAdapter::DBusObject() const {
    return G_DBUS_OBJECT_SKELETON(object_);
}

std::string NetworkDeviceAdapter::Path() const {
    return path_;
}

void NetworkDeviceAdapter::OnHandleConnect(AethercastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                           const gchar *role, gpointer user_data)
{
    boost::ignore_unused_variable_warning(skeleton);
    boost::ignore_unused_variable_warning(role);

    auto inst = static_cast<WeakKeepAlive<NetworkDeviceAdapter>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    g_object_ref(invocation);
    auto inv = make_shared_gobject(invocation);

    inst->service_->Connect(inst->device_, [inv](mcs::Error error) {
        if (error != Error::kNone) {
            g_dbus_method_invocation_return_error(inv.get(), G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                                                  "%s", mcs::ErrorToString(error).c_str());
            return;
        }

        g_dbus_method_invocation_return_value(inv.get(), nullptr);
    });
}

void NetworkDeviceAdapter::OnHandleDisconnect(AethercastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                              gpointer user_data)
{
    boost::ignore_unused_variable_warning(skeleton);

    auto inst = static_cast<WeakKeepAlive<NetworkDeviceAdapter>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    g_object_ref(invocation);
    auto inv = make_shared_gobject(invocation);

    inst->service_->Disconnect(inst->device_, [inv](mcs::Error error) {
        if (error != Error::kNone) {
            g_dbus_method_invocation_return_error(inv.get(), G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                                                  "%s", mcs::ErrorToString(error).c_str());
            return;
        }

        g_dbus_method_invocation_return_value(inv.get(), nullptr);
    });
}

} // namespace mcs
