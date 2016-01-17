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

#include <memory>

#include <mcs/keep_alive.h>
#include <mcs/logger.h>

#include "p2pdeviceskeleton.h"

namespace w11tng {
namespace testing {

P2PDeviceSkeleton::Ptr P2PDeviceSkeleton::FinalizeConstruction() {
    auto sp = shared_from_this();

    g_signal_connect_data(skeleton_.get(), "handle-find",
         G_CALLBACK(&P2PDeviceSkeleton::OnHandleFind), new mcs::WeakKeepAlive<P2PDeviceSkeleton>(shared_from_this()),
         [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<P2PDeviceSkeleton>*>(data); }, GConnectFlags(0));

    g_signal_connect_data(skeleton_.get(), "handle-stop-find",
         G_CALLBACK(&P2PDeviceSkeleton::OnHandleStopFind), new mcs::WeakKeepAlive<P2PDeviceSkeleton>(shared_from_this()),
         [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<P2PDeviceSkeleton>*>(data); }, GConnectFlags(0));

    return sp;
}

P2PDeviceSkeleton::Ptr P2PDeviceSkeleton::Create(const std::string &object_path) {
    return std::shared_ptr<P2PDeviceSkeleton>(new P2PDeviceSkeleton(object_path))->FinalizeConstruction();
}

P2PDeviceSkeleton::P2PDeviceSkeleton(const std::string &object_path) :
    BaseSkeleton(wpa_supplicant_interface_p2_pdevice_skeleton_new(), object_path) {
}

P2PDeviceSkeleton::~P2PDeviceSkeleton() {
}

gboolean P2PDeviceSkeleton::OnHandleFind(WpaSupplicantInterfaceP2PDevice *device, GDBusMethodInvocation *invocation, GVariant *properties, gpointer user_data) {
    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceSkeleton>*>(user_data)->GetInstance().lock();

    MCS_DEBUG("");

    if (not inst) {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Invalid state");
        return TRUE;
    }

    if (auto sp = inst->delegate_.lock())
        sp->OnFind();

    g_dbus_method_invocation_return_value(invocation, nullptr);

    return TRUE;
}

gboolean P2PDeviceSkeleton::OnHandleStopFind(WpaSupplicantInterfaceP2PDevice *device, GDBusMethodInvocation *invocation, gpointer user_data) {
    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceSkeleton>*>(user_data)->GetInstance().lock();

    MCS_DEBUG("");

    if (not inst) {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Invalid state");
        return TRUE;
    }

    if (auto sp = inst->delegate_.lock())
        sp->OnStopFind();

    g_dbus_method_invocation_return_value(invocation, nullptr);

    return TRUE;
}

void P2PDeviceSkeleton::EmitDeviceFound(const std::string &path) {
    wpa_supplicant_interface_p2_pdevice_emit_device_found(skeleton_.get(), path.c_str());
}

void P2PDeviceSkeleton::EmitDeviceLost(const std::string &path) {
    wpa_supplicant_interface_p2_pdevice_emit_device_lost(skeleton_.get(), path.c_str());
}

void P2PDeviceSkeleton::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

} // namespace testing
} // namespace w11tng
