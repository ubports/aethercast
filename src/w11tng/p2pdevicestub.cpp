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
#include <mcs/dbushelpers.h>

#include "p2pdevicestub.h"

namespace w11tng {

std::string P2PDeviceStub::StatusToString(Status status) {
    switch (status) {
    case Status::kSucccesAcceptedByUser:
    case Status::kSuccess:
        return "Success";
    case Status::kInformationIsCurrentlyUnavailable:
        return "Information is currently unavailable";
    case Status::kIncompatibleParameters:
        return "Incompatible parameters";
    case Status::kLimitReached:
        return "Limit reached";
    case Status::kInvalidParameter:
        return "Invalid parameter";
    case Status::kUnableToAccommodateRequest:
        return "Unable to accommodate request";
    case Status::kProtcolErrorOrDisruptiveBehavior:
        return "Protocol error or disruptive behavior";
    case Status::kNoCommonChannel:
        return "No common channels";
    case Status::kUnknownP2PGroup:
        return "Unknown P2P group";
    case Status::kBothGOIntent15:
        return "Both P2P devices indicated an intent of 15 in group owner negotiation";
    case Status::kIncompatibleProvisioningMethod:
        return "Incompatible provisioning method";
    case Status::kRejectByUser:
        return "Rejected by user";
    default:
        break;
    }

    return "Failed: unknown error";
}

std::string P2PDeviceStub::PropertyToString(Property property) {
    switch (property) {
    case Property::kPeerObject:
        return "peer_object";
    case Property::kStatus:
        return "status";
    case Property::kFrequency:
        return "frequency";
    case Property::kFrequencyList:
        return "frequency_list";
    case Property::kWpsMethod:
        return "wps_method";
    default:
        break;
    }
    return "unknown";
}

P2PDeviceStub::WpsMethod P2PDeviceStub::WpsMethodFromString(const std::string &wps_method) {
    if (wps_method == "pin")
        return WpsMethod::kPin;
    // In all other cases we directly fallback to PBC
    return WpsMethod::kPbc;
}

std::string P2PDeviceStub::WpsMethodToString(WpsMethod wps_method) {
    switch (wps_method) {
    case WpsMethod::kPbc:
        return "pbc";
    case WpsMethod::kPin:
        return "pin";
    default:
        break;
    }
    return "";
}

P2PDeviceStub::Ptr P2PDeviceStub::Create(const std::string &object_path, const std::weak_ptr<P2PDeviceStub::Delegate> &delegate) {
    return std::shared_ptr<P2PDeviceStub>(new P2PDeviceStub(delegate))->FinalizeConstruction(object_path);
}

std::shared_ptr<P2PDeviceStub> P2PDeviceStub::FinalizeConstruction(const std::string &object_path) {
    auto sp = shared_from_this();

    GError *error = nullptr;
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!connection_) {
        MCS_ERROR("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    wpa_supplicant_interface_p2_pdevice_proxy_new(connection_.get(),
                                                  G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                  kBusName,
                                                  object_path.c_str(),
                                                  nullptr,
                                                  [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        inst->proxy_.reset(wpa_supplicant_interface_p2_pdevice_proxy_new_finish(res, &error));
        if (!inst->proxy_) {
            MCS_ERROR("Failed to connect with P2P interface: %s", error->message);
            g_error_free(error);
            return;
        }

        inst->ConnectSignals();

        MCS_DEBUG("Successfully setup P2P device proxy");

        if (auto sp = inst->delegate_.lock())
            sp->OnP2PDeviceReady();

        if (auto sp = inst->delegate_.lock())
            sp->OnP2PDeviceChanged();

    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});

    return sp;
}

P2PDeviceStub::P2PDeviceStub(const std::weak_ptr<P2PDeviceStub::Delegate> &delegate) :
    delegate_(delegate),
    scan_timeout_source_(0) {
}

P2PDeviceStub::~P2PDeviceStub() {
}

void P2PDeviceStub::OnDeviceFound(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, gpointer user_data) {
    MCS_DEBUG("Found device %s", path);

    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    if (auto sp = inst->delegate_.lock())
        sp->OnDeviceFound(std::string(path));
}

void P2PDeviceStub::OnDeviceLost(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, gpointer user_data) {
    MCS_DEBUG("Lost device %s", path);

    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    if (not inst)
        return;

    if (auto sp = inst->delegate_.lock())
        sp->OnDeviceLost(std::string(path));
}

void P2PDeviceStub::OnGONegotiationSuccess(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data) {
    MCS_DEBUG("");

    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    std::string peer_path;
    GroupOwnerNegotiationResult result;

    mcs::DBusHelpers::ParseDictionary(properties, [&](const std::string &name, GVariant *value) {
        if (name == PropertyToString(Property::kStatus)) {
            const auto v = g_variant_get_variant(value);
            if (g_variant_is_of_type(v, G_VARIANT_TYPE("i")))
                result.status = static_cast<Status>(g_variant_get_int32(v));
        }
        else if (name == PropertyToString(Property::kPeerObject)) {
            const auto v = g_variant_get_variant(value);
            if (g_variant_is_of_type(v, G_VARIANT_TYPE("s")))
                peer_path = g_variant_get_string(v, nullptr);
        }
        else if (name == PropertyToString(Property::kFrequency)) {
            const auto v = g_variant_get_variant(value);
            if (g_variant_is_of_type(v, G_VARIANT_TYPE("i")))
                result.oper_freq = g_variant_get_int32(v);
        }
        else if (name == PropertyToString(Property::kFrequencyList)) {
            const auto v = g_variant_get_variant(value);
            for (int n = 0; n < g_variant_n_children(v); n++) {
                Frequency freq = 0;
                g_variant_get_child(v, n, "i", &freq);
                result.frequencies.insert(freq);
            }
        }
        else if (name == PropertyToString(Property::kWpsMethod)) {
            const auto v = g_variant_get_variant(value);
            if (g_variant_is_of_type(v, G_VARIANT_TYPE("s")))
                result.wps_method = WpsMethodFromString(g_variant_get_string(v, nullptr));
        }
    });

    if (peer_path.length() == 0)
        return;

    if (auto sp = inst->delegate_.lock())
        sp->OnGroupOwnerNegotiationSuccess(peer_path, result);
}

void P2PDeviceStub::OnGONegotiationFailure(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data) {
    MCS_DEBUG("");

    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    std::string peer_path;
    GroupOwnerNegotiationResult result;

    mcs::DBusHelpers::ParseDictionary(properties, [&](const std::string &name, GVariant *value) {
        if (name == "peer_object")
            peer_path = g_variant_get_string(g_variant_get_variant(value), nullptr);
        else if (name == "status")
            result.status = static_cast<Status>(g_variant_get_int32(g_variant_get_variant(value)));
    });

    if (peer_path.length() == 0)
        return;

    if (auto sp = inst->delegate_.lock())
        sp->OnGroupOwnerNegotiationFailure(peer_path, result);
}

void P2PDeviceStub::OnGroupStarted(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data) {
    MCS_DEBUG("");

    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    std::string interface_object;
    std::string group_object;
    std::string role;

    mcs::DBusHelpers::ParseDictionary(properties, [&](const std::string &name, GVariant *value) {
        if (name == "interface_object")
            interface_object = g_variant_get_string(g_variant_get_variant(value), nullptr);
        else if (name == "group_object")
            group_object = g_variant_get_string(g_variant_get_variant(value), nullptr);
        else if (name == "role")
            role = g_variant_get_string(g_variant_get_variant(value), nullptr);
    });

    if (auto sp = inst->delegate_.lock())
        sp->OnGroupStarted(group_object, interface_object, role);
}

void P2PDeviceStub::OnGroupFinished(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data) {
    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    std::string interface_object;
    std::string group_object;

    mcs::DBusHelpers::ParseDictionary(properties, [&](const std::string &name, GVariant *value) {
        if (name == "interface_object")
            interface_object = g_variant_get_string(g_variant_get_variant(value), nullptr);
        else if (name == "group_object")
            group_object = g_variant_get_string(g_variant_get_variant(value), nullptr);
    });

    MCS_DEBUG("interface %s group %s", interface_object, group_object);

    if (auto sp = inst->delegate_.lock())
        sp->OnGroupFinished(group_object, interface_object);
}

void P2PDeviceStub::OnGroupRequest(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, int dev_passwd_id, gpointer user_data) {
    auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

    std::string peer_path(path);

    if (auto sp = inst->delegate_.lock())
        sp->OnGroupRequest(peer_path, dev_passwd_id);
}

void P2PDeviceStub::ConnectSignals() {
    auto sp = shared_from_this();

#define CONNECT_SIGNAL(name, callback) \
    g_signal_connect_data(proxy_.get(), name, \
                          G_CALLBACK(&P2PDeviceStub::callback), new mcs::WeakKeepAlive<P2PDeviceStub>(sp), \
                          [](gpointer data, GClosure *) { delete static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(data); }, \
                          GConnectFlags(0));

    CONNECT_SIGNAL("device-found", OnDeviceFound);
    CONNECT_SIGNAL("device-lost", OnDeviceLost);
    CONNECT_SIGNAL("gonegotiation-success", OnGONegotiationSuccess);
    CONNECT_SIGNAL("gonegotiation-failure", OnGONegotiationFailure);
    CONNECT_SIGNAL("group-started", OnGroupStarted);
    CONNECT_SIGNAL("group-finished", OnGroupFinished);
    CONNECT_SIGNAL("gonegotiation-request", OnGroupRequest);
}

void P2PDeviceStub::StartFindTimeout() {
    scan_timeout_source_ = g_timeout_add_seconds(scan_timeout_.count(), [](gpointer user_data) {
        auto inst = static_cast<mcs::WeakKeepAlive<P2PDeviceStub>*>(user_data)->GetInstance().lock();

        if (not inst)
            return FALSE;

        inst->scan_timeout_source_ = 0;
        inst->StopFind();

        if (auto sp = inst->delegate_.lock())
            sp->OnP2PDeviceChanged();

        return FALSE;
    }, new mcs::WeakKeepAlive<P2PDeviceStub>{shared_from_this()});
}

void P2PDeviceStub::StopFindTimeout() {
    if (scan_timeout_source_ == 0)
        return;

    g_source_remove(scan_timeout_source_);
    scan_timeout_source_ = 0;
    scan_timeout_ = std::chrono::seconds{0};

    if (auto sp = delegate_.lock())
        sp->OnP2PDeviceChanged();
}

void P2PDeviceStub::Find(const std::chrono::seconds &timeout) {
    if (!proxy_ || scan_timeout_source_ > 0)
        return;

    MCS_DEBUG("timeout %d", timeout.count());

    scan_timeout_ = timeout;

    // FIXME wpa-supplicant starts to implement the FindStopped signal starting
    // with 2.5 so we have to emulate the scan timeout until we can switch to it:
    // auto builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);
    // g_variant_builder_add(builder, "{sv}", "Timeout", g_variant_new_int32(timeout.count()));
    // auto arguments = g_variant_builder_end(builder);

    // glib is rather unhappy if we just pass a nullptr for the arguments if we
    // don't use them so lets create just an empty array
    auto arguments = g_variant_new_array(g_variant_type_new("{sv}"), nullptr, 0);

    wpa_supplicant_interface_p2_pdevice_call_find(proxy_.get(), arguments, nullptr,
                                                  [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        if (!wpa_supplicant_interface_p2_pdevice_call_find_finish(inst->proxy_.get(), res, &error)) {
            MCS_ERROR("Failed to start P2P discovery: %s", error->message);
            g_error_free(error);
            return;
        }

        inst->StartFindTimeout();

        if (auto sp = inst->delegate_.lock())
            sp->OnP2PDeviceChanged();

    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});
}

void P2PDeviceStub::StopFind() {
    if (!proxy_)
        return;

    MCS_DEBUG("");

    wpa_supplicant_interface_p2_pdevice_call_stop_find(proxy_.get(), nullptr,
                                                  [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        if (!wpa_supplicant_interface_p2_pdevice_call_stop_find_finish(inst->proxy_.get(), res, &error)) {
            MCS_ERROR("Failed to stop P2P discovery: %s", error->message);
            g_error_free(error);
            return;
        }
    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});

    StopFindTimeout();
}

bool P2PDeviceStub::Connect(const std::string &path, const std::int32_t intent) {
    MCS_DEBUG("");

    if (!proxy_ || path.length() == 0)
        return false;

    MCS_DEBUG("path %s", path);

    auto builder = g_variant_builder_new(G_VARIANT_TYPE_ARRAY);

    MCS_DEBUG("Using GO intent %d", intent);

    g_variant_builder_add(builder, "{sv}", "peer", g_variant_new_object_path(path.c_str()));
    // We support only WPS PBC for now
    g_variant_builder_add(builder, "{sv}", "wps_method", g_variant_new_string(WpsMethodToString(WpsMethod::kPbc).c_str()));
    g_variant_builder_add(builder, "{sv}", "go_intent", g_variant_new_int32(intent));

    auto arguments = g_variant_builder_end(builder);

    wpa_supplicant_interface_p2_pdevice_call_connect(proxy_.get(), arguments, nullptr,
                                                     [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        if (!wpa_supplicant_interface_p2_pdevice_call_connect_finish(inst->proxy_.get(), nullptr, res, &error)) {
            MCS_ERROR("Failed to connect with P2P device: %s", error->message);
            g_error_free(error);

            if (auto sp = inst->delegate_.lock())
                sp->OnPeerConnectFailed();

            return;
        }
    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});

    return true;
}

bool P2PDeviceStub::Disconnect() {
    MCS_DEBUG("");

    wpa_supplicant_interface_p2_pdevice_call_disconnect(proxy_.get(), nullptr,
                                                        [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        if (!wpa_supplicant_interface_p2_pdevice_call_disconnect_finish(inst->proxy_.get(), res, &error)) {
            MCS_ERROR("Failed to disconnect with P2P device: %s", error->message);
            g_error_free(error);
            return;
        }

    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});

    return true;
}

bool P2PDeviceStub::DisconnectSync() {
    MCS_DEBUG("");

    GError *error = nullptr;
    if (!wpa_supplicant_interface_p2_pdevice_call_disconnect_sync(proxy_.get(), nullptr, &error)) {
        MCS_ERROR("Failed to disconnect: %s", error->message);
        g_error_free(error);
        return false;
    }

    return true;
}

void P2PDeviceStub::Flush() {
    MCS_DEBUG("");

    wpa_supplicant_interface_p2_pdevice_call_flush(proxy_.get(), nullptr,
                                                   [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        if (!wpa_supplicant_interface_p2_pdevice_call_flush_finish(inst->proxy_.get(), res, &error)) {
            MCS_ERROR("Failed to flush P2P device state: %s", error->message);
            g_error_free(error);
            return;
        }

    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});
}

void P2PDeviceStub::Cancel() {
    MCS_DEBUG("");

    wpa_supplicant_interface_p2_pdevice_call_cancel(proxy_.get(), nullptr,
                                                   [](GObject *source, GAsyncResult *res, gpointer user_data) {

        auto inst = static_cast<mcs::SharedKeepAlive<P2PDeviceStub>*>(user_data)->ShouldDie();

        GError *error = nullptr;
        if (!wpa_supplicant_interface_p2_pdevice_call_cancel_finish(inst->proxy_.get(), res, &error)) {
            MCS_ERROR("Failed to cancel P2P device operation: %s", error->message);
            g_error_free(error);
            return;
        }

    }, new mcs::SharedKeepAlive<P2PDeviceStub>{shared_from_this()});
}

int DevTypeStringToBinary(const std::string &type, unsigned char dev_type[8]) {
    int length, pos, end;
    char b[3] = {};
    char *e = nullptr;

    end = type.length();
    for (length = pos = 0; type.c_str()[pos] != '\0' && length < 8; length++) {
        if (pos + 2 > end)
            return 0;

        b[0] = type.at(pos);
        b[1] = type.at(pos + 1);

        dev_type[length] = strtol(b, &e, 16);
        if (e && *e != '\0')
            return 0;

        pos += 2;
    }

    return 8;
}

void P2PDeviceStub::SetDeviceConfiguration(const std::string &device_name, const std::string &device_type) {
    MCS_DEBUG("name %s device type %s", device_name, device_type);

    auto builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

    g_variant_builder_add(builder, "{sv}", "DeviceName", g_variant_new_string(device_name.c_str()));

    unsigned char dev_type[8] = {};
    int len = DevTypeStringToBinary(device_type, dev_type);
    g_variant_builder_add(builder, "{sv}", "PrimaryDeviceType", g_variant_new_fixed_array(G_VARIANT_TYPE("y"), dev_type, len, 1));

    auto value = g_variant_builder_end(builder);

    wpa_supplicant_interface_p2_pdevice_set_p2_pdevice_config(proxy_.get(), value);
}

std::string P2PDeviceStub::ObjectPath() const {
    return std::string(g_dbus_proxy_get_object_path(G_DBUS_PROXY(proxy_.get())));
}

} // namespace w11tng
