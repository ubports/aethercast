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

#include <stdexcept>

#include "ac/forwardingcontroller.h"

namespace ac {
ForwardingMiracastController::ForwardingMiracastController(const MiracastController::Ptr& fwd) : fwd_{fwd} {
    if (not fwd_) {
        throw std::logic_error{"Cannot operate without a valid MiracastController instance."};
    }
}

void ForwardingMiracastController::SetDelegate(const std::weak_ptr<MiracastController::Delegate> &delegate) {
    fwd_->SetDelegate(delegate);
}

void ForwardingMiracastController::ResetDelegate() {
    fwd_->ResetDelegate();
}

void ForwardingMiracastController::Connect(const NetworkDevice::Ptr &device, ResultCallback callback) {
    fwd_->Connect(device, callback);
}

void ForwardingMiracastController::Disconnect(const NetworkDevice::Ptr &device, ResultCallback callback) {
    fwd_->Disconnect(device, callback);
}

void ForwardingMiracastController::DisconnectAll(ResultCallback callback) {
    fwd_->DisconnectAll(callback);
}

ac::Error ForwardingMiracastController::Scan(const std::chrono::seconds &timeout) {
    return fwd_->Scan(timeout);
}

NetworkDeviceState ForwardingMiracastController::State() const {
    return fwd_->State();
}

std::vector<NetworkManager::Capability> ForwardingMiracastController::Capabilities() const {
    return fwd_->Capabilities();
}

bool ForwardingMiracastController::Scanning() const {
    return fwd_->Scanning();
}

bool ForwardingMiracastController::Enabled() const {
    return fwd_->Enabled();
}

Error ForwardingMiracastController::SetEnabled(bool enabled) {
    return fwd_->SetEnabled(enabled);
}
}
