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
#include <iostream>

#include "ac/keep_alive.h"
#include "ac/logger.h"

#include "p2pdeviceskeleton.h"

namespace w11tng {
namespace testing {

P2PDeviceSkeleton::Ptr P2PDeviceSkeleton::FinalizeConstruction() {
    auto sp = shared_from_this();

    g_signal_connect_data(skeleton_.get(), "handle-find",
         G_CALLBACK(&P2PDeviceSkeleton::OnHandleFind), new ac::WeakKeepAlive<P2PDeviceSkeleton>(shared_from_this()),
         [](gpointer data, GClosure *) { delete static_cast<ac::WeakKeepAlive<P2PDeviceSkeleton>*>(data); }, GConnectFlags(0));

    g_signal_connect_data(skeleton_.get(), "handle-stop-find",
         G_CALLBACK(&P2PDeviceSkeleton::OnHandleStopFind), new ac::WeakKeepAlive<P2PDeviceSkeleton>(shared_from_this()),
         [](gpointer data, GClosure *) { delete static_cast<ac::WeakKeepAlive<P2PDeviceSkeleton>*>(data); }, GConnectFlags(0));

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
    auto inst = static_cast<ac::WeakKeepAlive<P2PDeviceSkeleton>*>(user_data)->GetInstance().lock();

    AC_DEBUG("");

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
    auto inst = static_cast<ac::WeakKeepAlive<P2PDeviceSkeleton>*>(user_data)->GetInstance().lock();

    AC_DEBUG("");

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

void P2PDeviceSkeleton::EmitGroupOwnerNegotiationSuccess(const std::string &path, const P2PDeviceStub::Status status,
                                                         const P2PDeviceStub::Frequency freq, const P2PDeviceStub::FrequencyList &freqs,
                                                         const P2PDeviceStub::WpsMethod wps_method) {
    auto builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(builder, "{sv}", "peer_object", g_variant_new_string(path.c_str()));
    g_variant_builder_add(builder, "{sv}", "status", g_variant_new_int32(static_cast<gint32>(status)));
    g_variant_builder_add(builder, "{sv}", "frequency", g_variant_new_int32(freq));

    auto freq_builder = g_variant_builder_new(G_VARIANT_TYPE("ai"));
    for (auto freq : freqs)
        g_variant_builder_add(freq_builder, "i", freq);

    g_variant_builder_add(builder, "{sv}", "frequency_list", g_variant_builder_end(freq_builder));
    g_variant_builder_add(builder, "{sv}", "wps_method", g_variant_new_string(P2PDeviceStub::WpsMethodToString(wps_method).c_str()));

    auto value = g_variant_builder_end(builder);

    wpa_supplicant_interface_p2_pdevice_emit_gonegotiation_success(skeleton_.get(), value);
}

void P2PDeviceSkeleton::EmitGroupOwnerNegotiationFailure(const std::string &path) {
    auto builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(builder, "{sv}", "peer_object", g_variant_new_string(path.c_str()));
    auto value = g_variant_builder_end(builder);
    wpa_supplicant_interface_p2_pdevice_emit_gonegotiation_failure(skeleton_.get(), value);
}

void P2PDeviceSkeleton::EmitGroupStarted(const std::string &group_path, const std::string &interface_path, const std::string &role) {
    auto builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(builder, "{sv}", "group_object", g_variant_new_string(group_path.c_str()));
    g_variant_builder_add(builder, "{sv}", "interface_object", g_variant_new_string(interface_path.c_str()));
    g_variant_builder_add(builder, "{sv}", "role", g_variant_new_string(role.c_str()));
    auto value = g_variant_builder_end(builder);
    wpa_supplicant_interface_p2_pdevice_emit_group_started(skeleton_.get(), value);
}

void P2PDeviceSkeleton::EmitGroupFinished(const std::string &group_path, const std::string &interface_path) {
    auto builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(builder, "{sv}", "group_object", g_variant_new_string(group_path.c_str()));
    g_variant_builder_add(builder, "{sv}", "interface_object", g_variant_new_string(interface_path.c_str()));
    auto value = g_variant_builder_end(builder);
    wpa_supplicant_interface_p2_pdevice_emit_group_finished(skeleton_.get(), value);
}

void P2PDeviceSkeleton::EmitGroupRequest(const std::string &path, int dev_passwd_id) {
    wpa_supplicant_interface_p2_pdevice_emit_gonegotiation_request(skeleton_.get(), path.c_str(), dev_passwd_id);
}

void P2PDeviceSkeleton::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

} // namespace testing
} // namespace w11tng
