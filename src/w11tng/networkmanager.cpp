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

#include <algorithm>

#include <mcs/logger.h>

#include "networkmanager.h"

namespace w11tng {

mcs::NetworkManager::Ptr NetworkManager::Create() {
    return std::shared_ptr<NetworkManager>(new NetworkManager())->FinalizeConstruction();
}

std::shared_ptr<NetworkManager> NetworkManager::FinalizeConstruction() {
    auto sp = shared_from_this();

    p2p_device_ = P2PDeviceStub::Create(sp);

    return sp;
}

NetworkManager::NetworkManager() {
}

NetworkManager::~NetworkManager() {
}

void NetworkManager::OnSystemReady() {
}

void NetworkManager::OnSystemKilled() {
}

void NetworkManager::SetDelegate(mcs::NetworkManager::Delegate *delegate) {
    delegate_ = delegate;
}

bool NetworkManager::Setup() {
    return false;
}

void NetworkManager::Scan(const std::chrono::seconds &timeout) {
    p2p_device_->Find(timeout);
}

NetworkDevice::Ptr NetworkManager::FindDevice(const std::string &address) {
    for (auto iter : devices_) {
        if (iter.second->Address() == address)
            return iter.second;
    }
    return NetworkDevice::Ptr{};
}

bool NetworkManager::Connect(const mcs::NetworkDevice::Ptr &device) {
    MCS_DEBUG("address %s", device->Address());

    // Lets check here if we really own this device and if yes then we
    // select our own instance of it rather than relying on the input
    auto d = FindDevice(device->Address());
    if (!d)
        return false;

    return p2p_device_->Connect(d);
}

bool NetworkManager::Disconnect(const mcs::NetworkDevice::Ptr &device) {
    // Lets check here if we really own this device and if yes then we
    // select our own instance of it rather than relying on the input
    auto d = FindDevice(device->Address());
    if (!d)
        return false;


    return p2p_device_->Disconnect();
}

void NetworkManager::SetWfdSubElements(const std::list<std::string> &elements) {
}

std::vector<mcs::NetworkDevice::Ptr> NetworkManager::Devices() const {
    std::vector<mcs::NetworkDevice::Ptr> values;
    std::transform(devices_.begin(), devices_.end(),
                   std::back_inserter(values),
                   [=](const std::pair<std::string,w11tng::NetworkDevice::Ptr> &value) {
        return value.second;
    });
    return values;
}

mcs::IpV4Address NetworkManager::LocalAddress() const {
    return mcs::IpV4Address();
}

bool NetworkManager::Running() const {
    return p2p_device_->Connected();
}

bool NetworkManager::Scanning() const {
    return p2p_device_->Scanning();
}

void NetworkManager::OnChanged() {
    if (delegate_)
        delegate_->OnChanged();
}

void NetworkManager::OnDeviceFound(const std::string &path) {
    if (devices_.find(path) != devices_.end())
        return;

    auto device = NetworkDevice::Create(path);
    device->SetDelegate(shared_from_this());
    devices_[path] = device;

    // NOTE: OnDeviceFound will be send to delegate once the device
    // reports through OnDeviceReady that its ready for operation.
}

void NetworkManager::OnDeviceLost(const std::string &path) {
    if (devices_.find(path) == devices_.end())
        return;

    auto device = devices_[path];
    devices_.erase(path);

    if (delegate_)
        delegate_->OnDeviceLost(device);
}

void NetworkManager::OnDeviceChanged(const NetworkDevice::Ptr &device) {
    if (delegate_)
        delegate_->OnDeviceChanged(device);
}

void NetworkManager::OnDeviceReady(const NetworkDevice::Ptr &device) {
    if (delegate_)
        delegate_->OnDeviceFound(device);
}

} // namespace w11tng
