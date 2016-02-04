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

#include <mcs/logger.h>

#include "networkdevice.h"

namespace w11tng {

NetworkDevice::Ptr NetworkDevice::Create(const std::string &object_path) {
    return std::shared_ptr<NetworkDevice>(new NetworkDevice(object_path))->FinalizeConstruction();
}

NetworkDevice::Ptr NetworkDevice::FinalizeConstruction() {
    auto sp = shared_from_this();

    peer_->SetDelegate(sp);

    return sp;
}

NetworkDevice::NetworkDevice(const std::string &object_path) :
    object_path_(object_path),
    peer_(PeerStub::Create(object_path)),
    state_(mcs::kIdle) {
}

NetworkDevice::~NetworkDevice() {
}

void NetworkDevice::SetDelegate(const std::weak_ptr<Delegate> delegate) {
    delegate_ = delegate;
}

void NetworkDevice::ResetDelegate() {
    delegate_.reset();
}

void NetworkDevice::SyncWithPeer()
{
    address_ = peer_->Address();
    name_ = peer_->Name();
}

void NetworkDevice::OnPeerChanged() {
    MCS_DEBUG("Peer properties changed");
    SyncWithPeer();

    if (auto sp = delegate_.lock())
        sp->OnDeviceChanged(shared_from_this());
}

void NetworkDevice::OnPeerReady() {
    SyncWithPeer();

    if (auto sp = delegate_.lock())
        sp->OnDeviceReady(shared_from_this());
}

void NetworkDevice::SetAddress(const mcs::MacAddress &address) {
    address_ = address;
}

void NetworkDevice::SetIpV4Address(const mcs::IpV4Address &address) {
    ip_address_ = address;
}

void NetworkDevice::SetName(const std::string &name) {
    name_ = name;
}

void NetworkDevice::SetState(mcs::NetworkDeviceState state) {
    state_ = state;
}

void NetworkDevice::SetSupportedRoles(const std::vector<mcs::NetworkDeviceRole> roles) {
    supported_roles_ = roles;
}

void NetworkDevice::SetRole(const std::string &role) {
    role_ = role;
}

mcs::MacAddress NetworkDevice::Address() const {
    return address_;
}

mcs::IpV4Address NetworkDevice::IPv4Address() const {
    return ip_address_;
}

std::string NetworkDevice::Name() const {
    return name_;
}

mcs::NetworkDeviceState NetworkDevice::State() const {
    return state_;
}

std::vector<mcs::NetworkDeviceRole> NetworkDevice::SupportedRoles() const {
    return supported_roles_;
}

std::string NetworkDevice::ObjectPath() const {
    return object_path_;
}

std::string NetworkDevice::Role() const {
    return role_;
}

} // namespace w11tng
