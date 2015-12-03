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

#include "wfddeviceinfo.h"

#include <iostream>

namespace mcs {
std::ostream& operator<<(std::ostream &out, WfdDeviceType type) {
    switch (type) {
    case WfdDeviceType::kSource:
        return out << "source";
    case WfdDeviceType::kPrimarySink:
        return out << "primary-sink";
    case WfdDeviceType::kSecondarySink:
        return out << "secondary-sink";
    case WfdDeviceType::kSourceOrPrimarySink:
        return out << "source-or-primary-sink";
    }

    return out << "default";
}

WfdDeviceType WfdDeviceInfo::TypeFromInfoFlags(uint flags) {
    return static_cast<WfdDeviceType>(flags & Flag::type);
}

bool WfdDeviceInfo::IsSupportedSink() const {
    return device_type_ == WfdDeviceType::kPrimarySink || device_type_ == WfdDeviceType::kSourceOrPrimarySink;
}
}
