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

#ifndef W11TNG_NETWORKDEVICE_H_
#define W11TNG_NETWORKDEVICE_H_

#include <memory>

#include <mcs/networkdevice.h>
#include <mcs/shared_gobject.h>

extern "C" {
#include "wpasupplicantinterface.h"
}

#include "peerstub.h"

namespace w11tng {

class NetworkDevice : public std::enable_shared_from_this<NetworkDevice>,
                      public mcs::NetworkDevice,
                      public PeerStub::Delegate {
public:
    typedef std::shared_ptr<NetworkDevice> Ptr;

    class Delegate : public mcs::NonCopyable {
    public:
        virtual void OnDeviceReady(const NetworkDevice::Ptr &device) = 0;
        virtual void OnDeviceChanged(const NetworkDevice::Ptr &device) = 0;
    };

    static Ptr Create(const std::string &object_path);

    ~NetworkDevice();

    void SetDelegate(const std::weak_ptr<Delegate> delegate);
    void ResetDelegate();

    void SetAddress(const mcs::MacAddress &address);
    void SetIpV4Address(const mcs::IpV4Address &address);
    void SetName(const std::string &name);
    void SetState(mcs::NetworkDeviceState state);
    void SetSupportedRoles(const std::vector<mcs::NetworkDeviceRole> roles);

    mcs::MacAddress Address() const override;
    mcs::IpV4Address IPv4Address() const override;
    std::string Name() const override;
    mcs::NetworkDeviceState State() const override;
    std::vector<mcs::NetworkDeviceRole> SupportedRoles() const override;

    std::string ObjectPath() const;

    void OnPeerChanged() override;
    void OnPeerReady() override;

    void SetRole(const std::string &role);
    std::string Role() const;

private:
    NetworkDevice(const std::string &object_path);
    Ptr FinalizeConstruction();
    void SyncWithPeer();

private:
    std::weak_ptr<Delegate> delegate_;
    mcs::MacAddress address_;
    mcs::IpV4Address ip_address_;
    std::string name_;
    mcs::NetworkDeviceState state_;
    std::vector<mcs::NetworkDeviceRole> supported_roles_;
    std::string object_path_;
    std::shared_ptr<PeerStub> peer_;
    std::string role_;
};

} // namespace w11tng

#endif
