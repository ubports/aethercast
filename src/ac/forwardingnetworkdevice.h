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

#ifndef FORWARDINGNETWORKDEVICE_H_
#define FORWARDINGNETWORKDEVICE_H_

#include "networkdevice.h"

namespace ac {

class ForwardingNetworkDevice : public NetworkDevice {
public:
    ForwardingNetworkDevice(const NetworkDevice::Ptr& fwd);

    MacAddress Address() const override;
    IpV4Address IPv4Address() const override;
    std::string Name() const override;
    NetworkDeviceState State() const override;
    std::vector<NetworkDeviceRole> SupportedRoles() const override;

protected:
    const NetworkDevice::Ptr& Fwd() const;

private:
    NetworkDevice::Ptr fwd_;
};
} // namespace ac
#endif
