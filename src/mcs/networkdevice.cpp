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

#include "networkdevice.h"
#include "wfddeviceinfo.h"

namespace mcs {
std::string NetworkDevice::StateToStr(NetworkDeviceState state) {
    switch (state) {
    case kIdle:
        return "idle";
    case kFailure:
        return "failure";
    case kAssociation:
        return "connecting";
    case kConnected:
        return "connected";
    case kDisconnected:
        return "disconnected";
    default:
        break;
    }

    return "unknown";
}

std::string NetworkDevice::RoleToStr(NetworkDeviceRole role) {
    switch (role) {
    case kSource:
        return "source";
    case kSink:
        return "sink";
    default:
        break;
    }

    return "unknown";
}

NetworkDevice::NetworkDevice() :
    state_(kIdle) {
}

NetworkDevice::~NetworkDevice() {
}

MacAddress NetworkDevice::Address() const {
    return address_;
}

IpV4Address NetworkDevice::IPv4Address() const {
    return ipv4_address_;
}

std::string NetworkDevice::Name() const {
    return name_;
}

NetworkDeviceState NetworkDevice::State() const {
    return state_;
}

bool NetworkDevice::IsConnecting() const {
    return state_ == kConfiguration ||
            state_ == kAssociation;
}

std::string NetworkDevice::StateAsString() const {
    return StateToStr(state_);
}

std::vector<NetworkDeviceRole> NetworkDevice::SupportedRoles() const {
    return supported_roles_;
}

void NetworkDevice::SetAddress(const MacAddress &address) {
    address_ = address;
}

void NetworkDevice::SetIPv4Address(const IpV4Address &address) {
    ipv4_address_ = address;
}

void NetworkDevice::SetName(const std::string &name) {
    name_ = name;
}

void NetworkDevice::SetState(NetworkDeviceState state) {
    state_ = state;
}

void NetworkDevice::SetSupportedRoles(const std::vector<NetworkDeviceRole> roles) {
    supported_roles_ = roles;
}

} // namespace mcs
