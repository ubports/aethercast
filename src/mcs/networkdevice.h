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

#include "wfddeviceinfo.h"

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
    kUndecided,
    kGroupOwner,
    kGroupClient
};

class NetworkDevice {
public:
    typedef std::shared_ptr<NetworkDevice> Ptr;

    static std::string StateToStr(NetworkDeviceState State);

    NetworkDevice();
    ~NetworkDevice();

    std::string Address() const;
    std::string IPv4Address() const;
    std::string Name() const;
    NetworkDeviceState State() const;
    std::string StateAsString() const;
    WfdDeviceInfo DeviceInfo() const;
    int ConfigMethods() const;
    NetworkDeviceRole Role() const;

    void SetAddress(const std::string &address);
    void SetIPv4Address(const std::string &address);
    void SetName(const std::string &name);
    void SetState(NetworkDeviceState state);
    void SetWfdDeviceInfo(const WfdDeviceInfo &deviceInfo);
    void SetConfigMethods(int config_methods);
    void SetRole(NetworkDeviceRole role);

private:
    std::string name_;
    std::string address_;
    std::string ipv4_address_;
    NetworkDeviceState state_;
    WfdDeviceInfo wfd_device_info_;
    int config_methods_;
    NetworkDeviceRole role_;
};
} // namespace mcs
#endif
