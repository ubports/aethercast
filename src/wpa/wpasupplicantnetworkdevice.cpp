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

#include "wpasupplicantnetworkdevice.h"

WpaSupplicantNetworkDevice::WpaSupplicantNetworkDevice(const mcs::MacAddress &address, const std::string &name, const std::vector<mcs::NetworkDeviceRole> &roles) :
    address_(address),
    name_(name),
    state_(mcs::NetworkDeviceState::kIdle),
    supported_roles_(roles) {
}

mcs::MacAddress WpaSupplicantNetworkDevice::Address() const {
    return address_;
}

mcs::IpV4Address WpaSupplicantNetworkDevice::IPv4Address() const {
    return ip_address_;
}

std::string WpaSupplicantNetworkDevice::Name() const {
    return name_;
}

mcs::NetworkDeviceState WpaSupplicantNetworkDevice::State() const {
    return state_;
}

std::vector<mcs::NetworkDeviceRole> WpaSupplicantNetworkDevice::SupportedRoles() const {
    return supported_roles_;
}

mcs::MacAddress& WpaSupplicantNetworkDevice::Address() {
    return address_;
}

mcs::IpV4Address& WpaSupplicantNetworkDevice::IPv4Address() {
    return ip_address_;
}

std::string& WpaSupplicantNetworkDevice::Name() {
    return name_;
}

mcs::NetworkDeviceState& WpaSupplicantNetworkDevice::State() {
    return state_;
}

std::vector<mcs::NetworkDeviceRole>& WpaSupplicantNetworkDevice::SupportedRoles() {
    return supported_roles_;
}
