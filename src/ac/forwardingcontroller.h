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

#ifndef AC_FORWARDING_MIRACAST_CONTROLLER_H_
#define AC_FORWARDING_MIRACAST_CONTROLLER_H_

#include "ac/controller.h"

namespace ac {
class ForwardingController : public Controller
{
public:
    explicit ForwardingController(const Controller::Ptr& fwd);

    virtual void SetDelegate(const std::weak_ptr<Delegate> &delegate) override;
    virtual void ResetDelegate() override;

    virtual void Connect(const NetworkDevice::Ptr &device, ResultCallback callback) override;
    virtual void Disconnect(const NetworkDevice::Ptr &device, ResultCallback callback) override;

    virtual void DisconnectAll(ResultCallback callback) override;

    virtual ac::Error Scan(const std::chrono::seconds &timeout = std::chrono::seconds{30}) override;

    virtual NetworkDeviceState State() const override;
    virtual std::vector<NetworkManager::Capability> Capabilities() const override;
    virtual bool Scanning() const override;
    virtual bool Enabled() const override;

    virtual Error SetEnabled(bool enabled) override;

private:
    Controller::Ptr fwd_;
};
} // namespace ac
#endif
