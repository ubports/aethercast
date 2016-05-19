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

#include "ac/glib_wrapper.h"

#include "ac/keep_alive.h"
#include "ac/utils.h"
#include "ac/logger.h"

#include "ac/dbus/controllerskeleton.h"
#include "ac/dbus/errors.h"
#include "ac/dbus/helpers.h"

namespace {
constexpr const char *kManagerSkeletonInstanceKey{"controller-skeleton"};
}

namespace ac {
namespace dbus {
std::shared_ptr<ControllerSkeleton> ControllerSkeleton::Create(const std::shared_ptr<Controller> &controller) {
    return std::shared_ptr<ControllerSkeleton>(new ControllerSkeleton(controller))->FinalizeConstruction();
}

ControllerSkeleton::ControllerSkeleton(const std::shared_ptr<Controller> &controller) :
    ForwardingController(controller),
    manager_obj_(nullptr),
    bus_connection_(nullptr),
    bus_id_(0),
    object_manager_(nullptr) {
}

ControllerSkeleton::~ControllerSkeleton() {
    if (bus_id_ > 0)
        g_bus_unown_name(bus_id_);

    // We do not have to disconnect our handlers from:
    //   - handle-scan
    //   - handle-connect-sink
    // as we own the object emitting those signals.
}

void ControllerSkeleton::SyncProperties() {
    aethercast_interface_manager_set_state(manager_obj_.get(),
                                           NetworkDevice::StateToStr(State()).c_str());

    auto capabilities = Helpers::GenerateCapabilities(Capabilities());

    aethercast_interface_manager_set_capabilities(manager_obj_.get(), capabilities);

    g_strfreev(capabilities);

    aethercast_interface_manager_set_scanning(manager_obj_.get(), Scanning());
    aethercast_interface_manager_set_enabled(manager_obj_.get(), Enabled());
}

void ControllerSkeleton::OnStateChanged(NetworkDeviceState state) {
    if (!manager_obj_)
        return;

    aethercast_interface_manager_set_state(manager_obj_.get(),
                                           NetworkDevice::StateToStr(State()).c_str());
}

std::string ControllerSkeleton::GenerateDevicePath(const NetworkDevice::Ptr &device) const {
    std::string address = device->Address();
    std::replace(address.begin(), address.end(), ':', '_');
    // FIXME using kManagerPath doesn't seem to work. Fails at link time ...
    return ac::Utils::Sprintf("/org/aethercast/dev_%s", address.c_str());
}

void ControllerSkeleton::OnDeviceFound(const NetworkDevice::Ptr &device) {
    DEBUG("device %s", device->Address().c_str());

    auto path = GenerateDevicePath(device);
    auto adapter = NetworkDeviceSkeleton::Create(bus_connection_, path , device, shared_from_this());
    devices_.insert(std::pair<std::string,NetworkDeviceSkeleton::Ptr>(device->Address(), adapter));

    g_dbus_object_manager_server_export(object_manager_.get(), adapter->DBusObject());
}

void ControllerSkeleton::OnDeviceLost(const NetworkDevice::Ptr &device) {
    auto iter = devices_.find(device->Address());
    if (iter == devices_.end())
        return;

    g_dbus_object_manager_server_unexport(object_manager_.get(), iter->second->Path().c_str());

    devices_.erase(iter);
}

void ControllerSkeleton::OnDeviceChanged(const NetworkDevice::Ptr &peer) {
    auto iter = devices_.find(peer->Address());
    if (iter == devices_.end())
        return;

    iter->second->SyncProperties();
}

void ControllerSkeleton::OnChanged() {
    SyncProperties();
}

static std::string HyphenNameFromPropertyName(const std::string &property_name) {
    auto hyphen_name = property_name;
    // NOTE: Once we have more complex property names which have to
    // include a hyphen in its actual name we need to cover those
    // cases here.
    std::transform(hyphen_name.begin(), hyphen_name.end(), hyphen_name.begin(), ::tolower);
    return hyphen_name;
}

gboolean ControllerSkeleton::OnSetProperty(GDBusConnection *connection, const gchar *sender,
                                                   const gchar *object_path, const gchar *interface_name,
                                                   const gchar *property_name, GVariant *variant,
                                                   GError **error, gpointer user_data) {

    AethercastInterfaceManagerSkeleton *skeleton = AETHERCAST_INTERFACE_MANAGER_SKELETON(user_data);

    auto inst = g_object_get_data(G_OBJECT(skeleton), kManagerSkeletonInstanceKey);

    auto hyphen_name = HyphenNameFromPropertyName(property_name);
    auto pspec = g_object_class_find_property(G_OBJECT_GET_CLASS(skeleton),
                    hyphen_name.c_str());
    if (!pspec) {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                    "No property with name %s", property_name);
        return FALSE;
    }

    GValue value = G_VALUE_INIT;
    g_dbus_gvariant_to_gvalue(variant, &value);

    if (inst && hyphen_name == "enabled" &&
        g_variant_is_of_type(variant, G_VARIANT_TYPE_BOOLEAN)) {

        auto thiz = static_cast<WeakKeepAlive<ControllerSkeleton>*>(inst)->GetInstance().lock();
        if (!thiz) {
            g_set_error(error, AETHERCAST_ERROR, AETHERCAST_ERROR_INVALID_STATE, "Invalid state");
            return FALSE;
        }

        auto enabled = g_variant_get_boolean(variant);
        auto err = thiz->SetEnabled(static_cast<bool>(enabled));
        if (err != Error::kNone) {
            g_set_error(error, AETHERCAST_ERROR, AethercastErrorFromError(err),
                        "Failed to switch 'Enabled' property");
            return FALSE;
        }
    }

    g_object_set_property(G_OBJECT(skeleton), hyphen_name.c_str(), &value);
    g_value_unset(&value);

    return TRUE;
}

void ControllerSkeleton::OnNameAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    auto inst = static_cast<SharedKeepAlive<ControllerSkeleton>*>(user_data)->ShouldDie();

    inst->manager_obj_.reset(aethercast_interface_manager_skeleton_new());
    g_object_set_data(G_OBJECT(inst->manager_obj_.get()), kManagerSkeletonInstanceKey,
                      new WeakKeepAlive<ControllerSkeleton>(inst));

    // We override the property setter method of the skeleton's vtable
    // here to apply some more policy decisions when the user sets
    // specific properties which are state dependent.
    auto vtable = g_dbus_interface_skeleton_get_vtable(G_DBUS_INTERFACE_SKELETON(inst->manager_obj_.get()));
    vtable->set_property = &ControllerSkeleton::OnSetProperty;

    g_signal_connect_data(inst->manager_obj_.get(), "handle-scan",
                     G_CALLBACK(&ControllerSkeleton::OnHandleScan),
                     new WeakKeepAlive<ControllerSkeleton>(inst),
                     [](gpointer data, GClosure *) { delete static_cast<WeakKeepAlive<ControllerSkeleton>*>(data); },
                     GConnectFlags(0));

    g_signal_connect_data(inst->manager_obj_.get(), "handle-disconnect-all",
                     G_CALLBACK(&ControllerSkeleton::OnHandleDisconnectAll),
                     new WeakKeepAlive<ControllerSkeleton>(inst),
                     [](gpointer data, GClosure *) { delete static_cast<WeakKeepAlive<ControllerSkeleton>*>(data); },
                     GConnectFlags(0));

    inst->SyncProperties();

    g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(inst->manager_obj_.get()),
                                     connection, kManagerPath, nullptr);

    inst->object_manager_.reset(g_dbus_object_manager_server_new(kManagerPath));
    g_dbus_object_manager_server_set_connection(inst->object_manager_.get(), connection);

    INFO("Registered bus name %s", name);
}

gboolean ControllerSkeleton::OnHandleScan(AethercastInterfaceManager *skeleton,
                                              GDBusMethodInvocation *invocation, gpointer user_data) {
    boost::ignore_unused_variable_warning(skeleton);
    auto inst = static_cast<WeakKeepAlive<ControllerSkeleton>*>(user_data)->GetInstance().lock();

    if (not inst) {
        g_dbus_method_invocation_return_error(invocation, AETHERCAST_ERROR,
            AETHERCAST_ERROR_INVALID_STATE, "Invalid state");
        return TRUE;
    }

    INFO("Scanning for remote devices");

    const auto error = inst->Scan();
    if (error != ac::Error::kNone) {
        g_dbus_method_invocation_return_error(invocation, AETHERCAST_ERROR,
            AethercastErrorFromError(error), "%s", ac::ErrorToString(error).c_str());
        return TRUE;
    }

    g_dbus_method_invocation_return_value(invocation, nullptr);

    return TRUE;
}

gboolean ControllerSkeleton::OnHandleDisconnectAll(AethercastInterfaceManager *skeleton,
                                                           GDBusMethodInvocation *invocation, gpointer user_data) {
    boost::ignore_unused_variable_warning(skeleton);
    const auto inst = static_cast<WeakKeepAlive<ControllerSkeleton>*>(user_data)->GetInstance().lock();

    if (not inst) {
        g_dbus_method_invocation_return_error(invocation, AETHERCAST_ERROR,
            AETHERCAST_ERROR_INVALID_STATE, "Invalid state");
        return TRUE;
    }

    g_object_ref(invocation);
    auto inv = make_shared_gobject(invocation);

    inst->DisconnectAll([inv](ac::Error error) {
        if (error != Error::kNone) {
            g_dbus_method_invocation_return_error(inv.get(), AETHERCAST_ERROR,
                AethercastErrorFromError(error), "%s", ac::ErrorToString(error).c_str());
            return;
        }

        g_dbus_method_invocation_return_value(inv.get(), nullptr);
    });

    return TRUE;
}

std::shared_ptr<ControllerSkeleton> ControllerSkeleton::FinalizeConstruction() {
    auto sp = shared_from_this();

    GError *error = nullptr;
    bus_connection_ = make_shared_gobject(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!bus_connection_) {
        ERROR("Failed to connect with system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    bus_id_ = g_bus_own_name(G_BUS_TYPE_SYSTEM, kBusName, G_BUS_NAME_OWNER_FLAGS_NONE,
                   nullptr, &ControllerSkeleton::OnNameAcquired, nullptr, new SharedKeepAlive<ControllerSkeleton>{sp}, nullptr);
    if (bus_id_ == 0)
        WARNING("Failed to register bus name");

    SetDelegate(sp);
    return sp;
}
} // namespace dbus
} // namespace ac
