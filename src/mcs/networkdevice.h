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

#ifndef NETWORKDEVICE_H_
#define NETWORKDEVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "ip_v4_address.h"
#include "mac_address.h"

namespace mcs {
enum NetworkDeviceState {
    kIdle,
    kFailure,
    kAssociation,
    kConfiguration,
    kConnected,
    kDisconnected
};

enum NetworkDeviceRole {
    kSource,
    kSink
};

class NetworkDevice {
public:
    typedef std::shared_ptr<NetworkDevice> Ptr;

    static std::string StateToStr(NetworkDeviceState state);
    static std::string RoleToStr(NetworkDeviceRole role);

    NetworkDevice();
    ~NetworkDevice();

    MacAddress Address() const;
    IpV4Address IPv4Address() const;
    std::string Name() const;
    NetworkDeviceState State() const;
    std::string StateAsString() const;
    std::vector<NetworkDeviceRole> SupportedRoles() const;

    bool IsConnecting() const;

    void SetAddress(const MacAddress &address);
    void SetIPv4Address(const IpV4Address &Address);
    void SetName(const std::string &name);
    void SetState(NetworkDeviceState state);
    void SetSupportedRoles(const std::vector<NetworkDeviceRole> roles);

private:
    std::string name_;
    std::string address_;
    IpV4Address ipv4_address_;
    NetworkDeviceState state_;
    std::vector<NetworkDeviceRole> supported_roles_;
};
} // namespace mcs
#endif
