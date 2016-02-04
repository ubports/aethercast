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

#ifndef NETWORKMANAGERFACTORY_H_
#define NETWORKMANAGERFACTORY_H_

#include "networkmanager.h"

namespace mcs {

// Only here to make unit testing easier for the factory class
class NullNetworkManager : public mcs::NetworkManager {
public:
    void SetDelegate(Delegate * delegate) override;

    bool Setup() override;
    void Release() override;

    void Scan(const std::chrono::seconds &timeout) override;
    bool Connect(const NetworkDevice::Ptr &device) override;
    bool Disconnect(const NetworkDevice::Ptr &device) override;

    std::vector<NetworkDevice::Ptr> Devices() const override;
    IpV4Address LocalAddress() const override;
    bool Running() const override;
    bool Scanning() const override;

    void SetCapabilities(const std::vector<Capability> &capabilities) override;
    std::vector<Capability> Capabilities() const override;
};

class NetworkManagerFactory {
public:
    static NetworkManager::Ptr Create(const std::string &type = "");
};

} // namespace mcs

#endif
