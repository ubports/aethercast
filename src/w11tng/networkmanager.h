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

#ifndef W11TNG_NETWORKMANAGER_H_
#define W11TNG_NETWORKMANAGER_H_

#include <unordered_map>

#include <mcs/networkmanager.h>

#include "p2pdevicestub.h"

namespace w11tng {

class NetworkManager : public std::enable_shared_from_this<NetworkManager>,
                       public mcs::NetworkManager,
                       public P2PDeviceStub::Delegate,
                       public NetworkDevice::Delegate {
public:
    static mcs::NetworkManager::Ptr Create();

    ~NetworkManager();

    void SetDelegate(mcs::NetworkManager::Delegate * delegate) override;

    bool Setup() override;
    void Scan(const std::chrono::seconds &timeout) override;
    bool Connect(const mcs::NetworkDevice::Ptr &device) override;
    bool Disconnect(const mcs::NetworkDevice::Ptr &device) override;

    void SetWfdSubElements(const std::list<std::string> &elements) override;

    std::vector<mcs::NetworkDevice::Ptr> Devices() const override;
    mcs::IpV4Address LocalAddress() const override;
    bool Running() const override;
    bool Scanning() const override;

    void OnSystemReady() override;
    void OnSystemKilled() override;
    void OnChanged() override;
    void OnDeviceFound(const std::string &path) override;
    void OnDeviceLost(const std::string &path) override;

    void OnDeviceChanged(const NetworkDevice::Ptr &device) override;
    void OnDeviceReady(const NetworkDevice::Ptr &device) override;

private:
    NetworkManager();
    std::shared_ptr<NetworkManager> FinalizeConstruction();

    NetworkDevice::Ptr FindDevice(const std::string &address);

private:
    mcs::NetworkManager::Delegate *delegate_;
    std::shared_ptr<P2PDeviceStub> p2p_device_;
    std::unordered_map<std::string,w11tng::NetworkDevice::Ptr> devices_;
};

} // namespace w11tng

#endif
