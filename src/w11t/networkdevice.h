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

#ifndef WPASUPPLICANTNETWORKDEVICE_H_
#define WPASUPPLICANTNETWORKDEVICE_H_

#include <memory>
#include <string>
#include <vector>

#include <mcs/networkdevice.h>

namespace w11t {
class NetworkDevice : public mcs::NetworkDevice {
public:
    typedef std::shared_ptr<NetworkDevice> Ptr;

    NetworkDevice(const mcs::MacAddress &address, const std::string &name, const std::vector<mcs::NetworkDeviceRole> &roles);

    mcs::MacAddress Address() const override;
    mcs::IpV4Address IPv4Address() const override;
    std::string Name() const override;
    mcs::NetworkDeviceState State() const override;
    std::vector<mcs::NetworkDeviceRole> SupportedRoles() const override;

    mcs::MacAddress& Address();
    mcs::IpV4Address& IPv4Address();
    std::string& Name();
    mcs::NetworkDeviceState& State();
    std::vector<mcs::NetworkDeviceRole>& SupportedRoles();

private:
    mcs::MacAddress address_;
    mcs::IpV4Address ip_address_;
    std::string name_;
    mcs::NetworkDeviceState state_;
    std::vector<mcs::NetworkDeviceRole> supported_roles_;
};
}
#endif
