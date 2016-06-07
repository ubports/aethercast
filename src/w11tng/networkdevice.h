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

#include <ac/networkdevice.h>
#include <ac/shared_gobject.h>

extern "C" {
// Ignore all warnings coming from the external headers as we don't
// control them and also don't want to get any warnings from them
// which will only pollute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include "wpasupplicantinterface.h"
#pragma GCC diagnostic pop
}

#include "peerstub.h"

namespace w11tng {

class NetworkDevice : public std::enable_shared_from_this<NetworkDevice>,
                      public ac::NetworkDevice,
                      public PeerStub::Delegate {
public:
    typedef std::shared_ptr<NetworkDevice> Ptr;

    class Delegate : public ac::NonCopyable {
    public:
        virtual void OnDeviceReady(const NetworkDevice::Ptr &device) = 0;
        virtual void OnDeviceChanged(const NetworkDevice::Ptr &device) = 0;
    };

    static Ptr Create(const std::string &object_path);

    ~NetworkDevice();

    void SetDelegate(const std::weak_ptr<Delegate> delegate);
    void ResetDelegate();

    void SetAddress(const ac::MacAddress &address);
    void SetIpV4Address(const ac::IpV4Address &address);
    void SetName(const std::string &name);
    void SetState(ac::NetworkDeviceState state);
    void SetSupportedRoles(const std::vector<ac::NetworkDeviceRole> roles);

    ac::MacAddress Address() const override;
    ac::IpV4Address IPv4Address() const override;
    std::string Name() const override;
    ac::NetworkDeviceState State() const override;
    std::vector<ac::NetworkDeviceRole> SupportedRoles() const override;

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
    ac::MacAddress address_;
    ac::IpV4Address ip_address_;
    std::string name_;
    ac::NetworkDeviceState state_;
    std::vector<ac::NetworkDeviceRole> supported_roles_;
    std::string object_path_;
    std::shared_ptr<PeerStub> peer_;
    std::string role_;
};

} // namespace w11tng

#endif
