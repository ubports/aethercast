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

#include <boost/concept_check.hpp>

#include <w11tng/networkmanager.h>

#include "networkmanagerfactory.h"

#include "logger.h"
#include "utils.h"

namespace mcs {

void NullNetworkManager::SetDelegate(Delegate * delegate) {
    ERROR("Not implemented");
}

bool NullNetworkManager::Setup() {
    ERROR("Not implemented");
    return false;
}

void NullNetworkManager::Release() {
    ERROR("Not implemented");
}

void NullNetworkManager::Scan(const std::chrono::seconds &timeout) {
    ERROR("Not implemented");
}

bool NullNetworkManager::Connect(const NetworkDevice::Ptr &device) {
    ERROR("Not implemented");
    return false;
}

bool NullNetworkManager::Disconnect(const NetworkDevice::Ptr &device) {
    ERROR("Not implemented");
    return false;
}

std::vector<NetworkDevice::Ptr> NullNetworkManager::Devices() const {
    ERROR("Not implemented");
    return std::vector<NetworkDevice::Ptr>();
}

IpV4Address NullNetworkManager::LocalAddress() const {
    ERROR("Not implemented");
    return IpV4Address();
}

bool NullNetworkManager::Running() const {
    ERROR("Not implemented");
    return false;
}

bool NullNetworkManager::Scanning() const {
    ERROR("Not implemented");
    return false;
}

bool NullNetworkManager::Ready() const {
    ERROR("Not implemented");
    return false;
}

void NullNetworkManager::SetCapabilities(const std::vector<Capability> &capabilities) {
    ERROR("Not implemented");
}

std::vector<NetworkManager::Capability> NullNetworkManager::Capabilities() const {
    return std::vector<NetworkManager::Capability>{};
}

NetworkManager::Ptr NetworkManagerFactory::Create(const std::string &type) {
    auto final_type = type.length() > 0 ? type : Utils::GetEnvValue("AETHERCAST_NETWORK_MANAGER");

    DEBUG("Creating network manager of type %s", final_type.length() > 0 ? final_type : "w11tng");

    // We will always default to the w11t implementation if no invalid
    // type is specified.
    if (final_type == "w11tng" || final_type.length() == 0)
        return w11tng::NetworkManager::Create();

    return std::make_shared<NullNetworkManager>();
}

} // namespace mcs
