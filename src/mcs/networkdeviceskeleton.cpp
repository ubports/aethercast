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

#include "networkdeviceskeleton.h"
#include "utils.h"
#include "keep_alive.h"
#include "logger.h"
#include "dbushelpers.h"

namespace mcs {

NetworkDeviceSkeleton::Ptr NetworkDeviceSkeleton::Create(const SharedGObject<GDBusConnection> &connection, const std::string &path, const NetworkDevice::Ptr &device, const MiracastController::Ptr &controller) {
    return std::shared_ptr<NetworkDeviceSkeleton>(new NetworkDeviceSkeleton(connection, path, device, controller))->FinalizeConstruction();
}

NetworkDeviceSkeleton::NetworkDeviceSkeleton(const SharedGObject<GDBusConnection> &connection, const std::string &path, const NetworkDevice::Ptr &device, const MiracastController::Ptr &controller) :
    ForwardingNetworkDevice(device),
    connection_(connection),
    object_(make_shared_gobject(aethercast_interface_object_skeleton_new(path.c_str()))),
    path_(path),
    controller_(controller),
    device_iface_(aethercast_interface_device_skeleton_new()) {
}

std::shared_ptr<NetworkDeviceSkeleton> NetworkDeviceSkeleton::FinalizeConstruction() {
    auto sp = shared_from_this();

    g_signal_connect(device_iface_.get(), "handle-connect",
                     G_CALLBACK(&NetworkDeviceSkeleton::OnHandleConnect),
                     new WeakKeepAlive<NetworkDeviceSkeleton>{sp});
    g_signal_connect(device_iface_.get(), "handle-disconnect",
                     G_CALLBACK(&NetworkDeviceSkeleton::OnHandleDisconnect),
                     new WeakKeepAlive<NetworkDeviceSkeleton>{sp});

    SyncProperties();

    if (!object_)
        ERROR("Failed to create object for device %s", Address());
    else
        aethercast_interface_object_skeleton_set_device(object_.get(), device_iface_.get());

    return sp;
}

void NetworkDeviceSkeleton::SyncProperties() {
    aethercast_interface_device_set_address(device_iface_.get(), Address().c_str());
    aethercast_interface_device_set_name(device_iface_.get(), Name().c_str());
    aethercast_interface_device_set_state(device_iface_.get(), NetworkDevice::StateToStr(State()).c_str());

    auto capabilities = DBusHelpers::GenerateDeviceCapabilities(SupportedRoles());
    aethercast_interface_device_set_capabilities(device_iface_.get(), capabilities);
    g_strfreev(capabilities);
}

GDBusObjectSkeleton* NetworkDeviceSkeleton::DBusObject() const {
    return G_DBUS_OBJECT_SKELETON(object_.get());
}

std::string NetworkDeviceSkeleton::Path() const {
    return path_;
}

// TODO(tvoss,morphis): Refactor mcs::NetworkDevice to have Connect/Disconnect defined on its interfaces.
// It feels quite dirty to require both an instance of mcs::NetworkDevice and mcs::MiracastController to
// implement the connect/disconnect calls coming in via the bus. The complication then is the async handling of
// the invocation, as we will likely have to reach out to WPASupplicant for example (which is dispatched via the same
// event loop as we are). In addition, we should not start littering our public interfaces by handing down callbacks.
gboolean NetworkDeviceSkeleton::OnHandleConnect(AethercastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                           const gchar *role, gpointer user_data)
{
    boost::ignore_unused_variable_warning(skeleton);
    boost::ignore_unused_variable_warning(role);

    auto inst = static_cast<WeakKeepAlive<NetworkDeviceSkeleton>*>(user_data)->GetInstance().lock();

    if (not inst) {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Invalid state");
        return TRUE;
    }

    g_object_ref(invocation);
    auto inv = make_shared_gobject(invocation);

    inst->controller_->Connect(inst->Fwd(), [inv](mcs::Error error) {
        if (error != Error::kNone) {
            g_dbus_method_invocation_return_error(inv.get(), G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                                                  "%s", mcs::ErrorToString(error).c_str());
            return;
        }

        g_dbus_method_invocation_return_value(inv.get(), nullptr);
    });

    return TRUE;
}

gboolean NetworkDeviceSkeleton::OnHandleDisconnect(AethercastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                                   gpointer user_data)
{
    boost::ignore_unused_variable_warning(skeleton);

    auto inst = static_cast<WeakKeepAlive<NetworkDeviceSkeleton>*>(user_data)->GetInstance().lock();

    if (not inst) {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Invalid state");
        return TRUE;
    }

    g_object_ref(invocation);
    auto inv = make_shared_gobject(invocation);

    inst->controller_->Disconnect(inst->Fwd(), [inv](mcs::Error error) {
        if (error != Error::kNone) {
            g_dbus_method_invocation_return_error(inv.get(), G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                                                  "%s", mcs::ErrorToString(error).c_str());
            return;
        }

        g_dbus_method_invocation_return_value(inv.get(), nullptr);
    });

    return TRUE;
}

} // namespace mcs
