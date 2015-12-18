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
#include "non_copyable.h"

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

class NetworkDevice : public NonCopyable {
public:
    typedef std::shared_ptr<NetworkDevice> Ptr;

    static std::string StateToStr(NetworkDeviceState state);
    static std::string RoleToStr(NetworkDeviceRole role);

    virtual MacAddress Address() const = 0;
    virtual IpV4Address IPv4Address() const = 0;
    virtual std::string Name() const = 0;
    virtual NetworkDeviceState State() const = 0;
    virtual std::vector<NetworkDeviceRole> SupportedRoles() const = 0;

    bool IsConnecting() const;

protected:
    NetworkDevice() = default;
};
} // namespace mcs
#endif
