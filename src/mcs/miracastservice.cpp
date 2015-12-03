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
std::shared_ptr<MiracastService> MiracastService::create() {
    auto sp = std::shared_ptr<MiracastService>{new MiracastService{}};
    return sp->FinalizeConstruction();
}

MiracastService::MiracastService() :
    network_manager_(nullptr),
    current_state_(kIdle),
    source_(MiracastSourceManager::create()),
    current_peer_(nullptr) {
    // FIXME need to use a factory here for network manager construction
    network_manager_ = new WpaSupplicantNetworkManager(this);
    network_manager_->Setup();
}

std::shared_ptr<MiracastService> MiracastService::FinalizeConstruction() {
    auto sp = shared_from_this();
    source_->SetDelegate(sp);
    return sp;
}

MiracastService::~MiracastService() {
}

void MiracastService::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

void MiracastService::ResetDelegate() {
    delegate_.reset();
}

NetworkDeviceState MiracastService::State() const {
    return current_state_;
}

void MiracastService::OnClientDisconnected() {
    AdvanceState(kFailure);
    current_peer_.reset();
}

void MiracastService::AdvanceState(NetworkDeviceState new_state) {
    IpV4Address address;

    g_warning("AdvanceState newsstate %d current state %d", new_state, current_state_);

    switch (new_state) {
    case kAssociation:
        break;

    case kConnected:
        address = network_manager_->LocalAddress();
        source_->Setup(address, MIRACAST_DEFAULT_RTSP_CTRL_PORT);

        FinishConnectAttempt(true);

        break;

    case kFailure:
        if (current_state_ == kAssociation ||
                current_state_ == kConfiguration)
            FinishConnectAttempt(false, "Failed to connect remote device");

    case kDisconnected:
        if (current_state_ == kConnected)
            source_->Release();

        StartIdleTimer();
        break;

    case kIdle:
        break;

    default:
        break;
    }

    current_state_ = new_state;
    if (auto sp = delegate_.lock())
        sp->OnStateChanged(current_state_);
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
    if (auto sp = delegate_.lock())
        sp->OnDeviceFound(peer);
}

void MiracastService::OnDeviceLost(const NetworkDevice::Ptr &peer) {
    if (auto sp = delegate_.lock())
        sp->OnDeviceLost(peer);
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

void MiracastService::ConnectSink(const MacAddress &address, std::function<void(bool,std::string)> callback) {
    if (current_peer_.get()) {
        callback(false, "Already connected");
        return;
    }

    NetworkDevice::Ptr device;

    for (auto peer : network_manager_->Devices()) {
        if (peer->Address() != address)
            continue;

        device = peer;
        break;
    }

    if (!device) {
        callback(false, "Couldn't find device");
        return;
    }

    if (!network_manager_->Connect(device)) {
        callback(false, "Failed to connect with remote device");
        return;
    }

    current_peer_ = device;
    connect_callback_ = callback;
}

void MiracastService::Scan() {
    network_manager_->Scan();
}
} // namespace miracast
