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

#include "forwardingnetworkdevice.h"

namespace ac {

ForwardingNetworkDevice::ForwardingNetworkDevice(const NetworkDevice::Ptr& fwd) : fwd_(fwd) {
    if (not fwd_) {
        throw std::logic_error{"Cannot operate without a valid NetworkDevice instance."};
    }
}

MacAddress ForwardingNetworkDevice::Address() const {
    return fwd_->Address();
}

IpV4Address ForwardingNetworkDevice::IPv4Address() const {
    return fwd_->IPv4Address();
}

std::string ForwardingNetworkDevice::Name() const {
    return fwd_->Name();
}

NetworkDeviceState ForwardingNetworkDevice::State() const {
    return fwd_->State();
}

std::vector<NetworkDeviceRole> ForwardingNetworkDevice::SupportedRoles() const {
    return fwd_->SupportedRoles();
}

const NetworkDevice::Ptr& ForwardingNetworkDevice::Fwd() const {
    return fwd_;
}
} // namespace ac
