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

#ifndef NETWORKMANAGER_H_
#define NETWORKMANAGER_H_

#include <boost/core/noncopyable.hpp>

#include <list>
#include <vector>

#include "networkdevice.h"
#include "non_copyable.h"

namespace mcs {
class NetworkManager : private mcs::NonCopyable {
public:
    class Delegate : private mcs::NonCopyable {
    public:
        virtual void OnDeviceFound(const NetworkDevice::Ptr &peer) = 0;
        virtual void OnDeviceLost(const NetworkDevice::Ptr &peer) = 0;
        virtual void OnDeviceStateChanged(const NetworkDevice::Ptr &peer) = 0;

    protected:
        Delegate() = default;
    };

    virtual bool Setup() = 0;

    virtual void SetWfdSubElements(const std::list<std::string> &elements) = 0;

    virtual void Scan(unsigned int timeout = 30) = 0;

    virtual std::vector<NetworkDevice::Ptr> Devices() const = 0;

    virtual int Connect(const std::string &address, bool persistent = true) = 0;
    virtual int DisconnectAll() = 0;

    virtual NetworkDeviceRole Role() const = 0;
    virtual std::string LocalAddress() const = 0;
    virtual bool Running() const = 0;

protected:
    NetworkManager() = default;
};
} // namespace mcs
#endif
