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

#include "miracastservice.h"
#include "wpasupplicantnetworkmanager.h"
#include "wfddeviceinfo.h"

#define MIRACAST_DEFAULT_RTSP_CTRL_PORT     7236

#define STATE_IDLE_TIMEOUT                  1000

namespace mcs {
MiracastService::MiracastService() :
    delegate_(nullptr),
    current_state_(kIdle),
    source_(this),
    current_peer_(nullptr) {
    // FIXME this really needs to move somewhere else as it's something
    // specific for some devices and somehow goes together with the
    // network manager implementation
    LoadWiFiFirmware();
}

MiracastService::~MiracastService() {
}

void MiracastService::SetDelegate(Delegate *delegate) {
    delegate_ = delegate;
}

NetworkDeviceState MiracastService::State() const {
    return current_state_;
}

void MiracastService::OnClientDisconnected() {
    AdvanceState(kFailure);
    current_peer_.reset();
}

gboolean MiracastService::OnRetryLoadFirmware(gpointer user_data) {
    auto inst = static_cast<MiracastService*>(user_data);
    inst->LoadWiFiFirmware();
}

void MiracastService::OnWiFiFirmwareLoaded(GDBusConnection *conn, GAsyncResult *res, gpointer user_data) {
    auto inst = static_cast<MiracastService*>(user_data);
    guint timeout = 500;
    GError *error = nullptr;

    GVariant *result = g_dbus_connection_call_finish(conn, res, &error);
    if (error) {
        g_warning("Failed to load required WiFi firmware: %s", error->message);
        timeout = 2000;
    }

    g_timeout_add(timeout, &MiracastService::OnRetryLoadFirmware, inst);
}

void MiracastService::LoadWiFiFirmware() {
    if (!g_file_test("/sys/class/net/p2p0/uevent", (GFileTest) (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {

        g_warning("Switching device WiFi chip firmware to get P2P support");

        auto conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);

        GVariant *params = g_variant_new("(os)", "/", "p2p");

        g_dbus_connection_call(conn, "fi.w1.wpa_supplicant1", "/fi/w1/wpa_supplicant1",
                               "fi.w1.wpa_supplicant1", "SetInterfaceFirmware", params,
                               nullptr, (GDBusCallFlags) G_DBUS_CALL_FLAGS_NONE, -1,
                               nullptr, (GAsyncReadyCallback) &MiracastService::OnWiFiFirmwareLoaded, this);

        return;
    }

    manager_ = new WpaSupplicantNetworkManager(this, "p2p0");
    manager_->Setup();
}


void MiracastService::AdvanceState(NetworkDeviceState new_state) {
    std::string address;

    g_warning("AdvanceState newsstate %d current state %d", new_state, current_state_);

    switch (new_state) {
    case kAssociation:
        break;

    case kConnected:
        // We've have to pick the right address we need to tell our source to
        // push all streaming data to.
        address = current_peer_->Address();
        if (manager_->Role() == kGroupOwner)
            address = manager_->LocalAddress();

        source_.Setup(address, MIRACAST_DEFAULT_RTSP_CTRL_PORT);

        FinishConnectAttempt(true);

        break;

    case kFailure:
        if (current_state_ == kAssociation ||
                current_state_ == kConfiguration)
            FinishConnectAttempt(false, "Failed to connect remote device");

    case kDisconnected:
        if (current_state_ == kConnected)
            source_.Release();

        StartIdleTimer();
        break;

    case kIdle:
        break;

    default:
        break;
    }

    current_state_ = new_state;
    if (delegate_)
        delegate_->OnStateChanged(current_state_);
}

void MiracastService::OnDeviceStateChanged(const NetworkDevice::Ptr &peer) {
    if (peer->State() == kConnected)
        AdvanceState(kConnected);
    else if (peer->State() == kDisconnected) {
        AdvanceState(kDisconnected);
        current_peer_.reset();
    }
    else if (peer->State() == kFailure) {
        AdvanceState(kFailure);
        current_peer_.reset();
        FinishConnectAttempt(false, "Failed to connect device");
    }
}

void MiracastService::OnDeviceFound(const NetworkDevice::Ptr &peer) {
    if (delegate_)
        delegate_->OnDeviceFound(peer);
}

void MiracastService::OnDeviceLost(const NetworkDevice::Ptr &peer) {
    if (delegate_)
        delegate_->OnDeviceLost(peer);
}

gboolean MiracastService::OnIdleTimer(gpointer user_data) {
    auto inst = static_cast<MiracastService*>(user_data);
    inst->AdvanceState(kIdle);
}

void MiracastService::StartIdleTimer() {
    g_timeout_add(STATE_IDLE_TIMEOUT, &MiracastService::OnIdleTimer, this);
}

void MiracastService::FinishConnectAttempt(bool success, const std::string &error_text) {
    if (connect_callback_)
        connect_callback_(success, error_text);

    connect_callback_ = nullptr;
}

void MiracastService::ConnectSink(const std::string &address, std::function<void(bool,std::string)> callback) {
    if (current_peer_.get()) {
        callback(false, "Already connected");
        return;
    }

    NetworkDevice::Ptr device;

    for (auto peer : manager_->Devices()) {
        if (peer->Address() != address)
            continue;

        device = peer;
        break;
    }

    if (!device) {
        callback(false, "Couldn't find device");
        return;
    }

    if (manager_->Connect(device->Address(), false) < 0) {
        callback(false, "Failed to connect with remote device");
        return;
    }

    current_peer_ = device;
    connect_callback_ = callback;
}

void MiracastService::Scan() {
    manager_->Scan();
}
} // namespace miracast
