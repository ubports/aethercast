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

#include "networkp2pdevice.h"
#include "wfddeviceinfo.h"

class NetworkP2pDevice::Private
{
public:
    Private() :
        state(Idle),
        configMethods(0)
    {
    }

    QString name;
    QString address;
    NetworkP2pDevice::State state;
    WfdDeviceInfo wfdDeviceInfo;
    int configMethods;
};

NetworkP2pDevice::NetworkP2pDevice() :
    QObject(nullptr),
    d(new Private)
{
}

NetworkP2pDevice::~NetworkP2pDevice()
{
}

QString NetworkP2pDevice::address() const
{
    return d->address;
}

QString NetworkP2pDevice::name() const
{
    return d->name;
}

NetworkP2pDevice::State NetworkP2pDevice::state() const
{
    return d->state;
}

QString NetworkP2pDevice::stateAsString() const
{
    switch (d->state) {
    case Idle:
        return "idle";
    case Failure:
        return "failure";
    case Connecting:
        return "connecting";
    case Connected:
        return "connected";
    case Disconnecting:
        return "disconnecting";
    default:
        break;
    }

    return "unknown";
}

WfdDeviceInfo NetworkP2pDevice::wfdDeviceInfo() const
{
    return d->wfdDeviceInfo;
}

int NetworkP2pDevice::configMethods() const
{
    return d->configMethods;
}

void NetworkP2pDevice::setAddress(const QString &address)
{
    d->address = address;
}

void NetworkP2pDevice::setName(const QString &name)
{
    d->name = name;
}

void NetworkP2pDevice::setState(State state)
{
    d->state = state;
}

void NetworkP2pDevice::setWfdDeviceInfo(const WfdDeviceInfo &wfdDeviceInfo)
{
    d->wfdDeviceInfo = wfdDeviceInfo;
}

void NetworkP2pDevice::setConfigMethods(int configMethods)
{
    d->configMethods = configMethods;
}
