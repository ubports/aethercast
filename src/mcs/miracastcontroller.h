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

#ifndef MIRACAST_CONTROLLER_H_
#define MIRACAST_CONTROLLER_H_

#include <chrono>
#include <memory>

#include "networkmanager.h"
#include "networkdevice.h"
#include "non_copyable.h"
#include "types.h"

namespace mcs {
class MiracastController : private mcs::NonCopyable
{
public:
    typedef std::shared_ptr<MiracastController> Ptr;

    class Delegate : private mcs::NonCopyable {
    public:
        virtual ~Delegate() { }

        virtual void OnStateChanged(NetworkDeviceState state) = 0;
        virtual void OnChanged() = 0;
        virtual void OnDeviceFound(const NetworkDevice::Ptr &device) = 0;
        virtual void OnDeviceLost(const NetworkDevice::Ptr &device) = 0;
        virtual void OnDeviceChanged(const NetworkDevice::Ptr &device) = 0;

    protected:
        Delegate() = default;
    };

    virtual void SetDelegate(const std::weak_ptr<Delegate> &delegate) = 0;
    virtual void ResetDelegate() = 0;

    virtual void Connect(const NetworkDevice::Ptr &device, ResultCallback callback) = 0;
    virtual void Disconnect(const NetworkDevice::Ptr &device, ResultCallback callback) = 0;

    virtual void DisconnectAll(ResultCallback callback) = 0;

    virtual mcs::Error Scan(const std::chrono::seconds &timeout = std::chrono::seconds{30}) = 0;

    virtual NetworkDeviceState State() const = 0;
    virtual std::vector<NetworkManager::Capability> Capabilities() const = 0;
    virtual bool Scanning() const = 0;
    virtual bool Enabled() const = 0;

    virtual Error SetEnabled(bool enabled) = 0;

protected:
    MiracastController() = default;
};
} // namespace mcs
#endif
