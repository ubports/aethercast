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

#include "networkdevice.h"

namespace ac {
std::string NetworkDevice::StateToStr(NetworkDeviceState state) {
    switch (state) {
    case kIdle:
        return "idle";
    case kFailure:
        return "failure";
    case kAssociation:
        return "association";
    case kConfiguration:
        return "configuration";
    case kConnected:
        return "connected";
    case kDisconnected:
        return "disconnected";
    default:
        break;
    }

    return "unknown";
}

std::string NetworkDevice::RoleToStr(NetworkDeviceRole role) {
    switch (role) {
    case kSource:
        return "source";
    case kSink:
        return "sink";
    default:
        break;
    }

    return "unknown";
}

bool NetworkDevice::IsConnecting() const {
    return State() == kConfiguration ||
            State() == kAssociation;
}
} // namespace ac
