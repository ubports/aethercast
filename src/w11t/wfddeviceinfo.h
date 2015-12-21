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

#ifndef WFDDEVICEINFO_H_
#define WFDDEVICEINFO_H_

#include <iosfwd>
#include <string>

namespace w11t {
enum class WpsConfig {
    kDisplay = 0x0008,
    kPushbutton = 0x0080,
    kKeypad = 0x0100
};

enum class WfdDeviceType {
    unknown = -1,
    kSource = 0x0,
    kPrimarySink = 0x1,
    kSecondarySink = 0x2,
    kSourceOrPrimarySink = 0x3
};

std::ostream& operator<<(std::ostream &, WfdDeviceType);

struct WfdDeviceInfo
{
    static WfdDeviceType TypeFromInfoFlags(uint flags);

    static WfdDeviceInfo Parse(const std::string &str);

    enum Flag {
        type = 0x3,
        coupled_sink_support_at_source = 0x4,
        coupled_sink_support_at_sink = 0x8,
        session_available = 0x30,
        session_available_bit1 = 0x10,
        session_available_bit2 = 0x20
    };

    bool IsSupported() const;
    bool IsSupportedSink() const;
    bool IsSupportedSource() const;

    WfdDeviceType device_type_ = WfdDeviceType::unknown;
    ushort ctrl_port_ = 0;
    uint max_tput_ = 0;
};
}
#endif
