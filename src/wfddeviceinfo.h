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

#include <string>

/*
 * WFD Device Type
 */

#define WFD_DEVICE_TYPE_SOURCE                     0x0
#define WFD_DEVICE_TYPE_PRIMARY_SINK               0x1
#define WFD_DEVICE_TYPE_SECONDARY_SINK             0x2
#define WFD_DEVICE_TYPE_SOURCE_OR_PRIMARY_SINK     0x3

/*
 * WFD Device Information Bitmap
 */

#define WFD_DEVICE_INFO_DEVICE_TYPE                         0x3
#define WFD_DEVICE_INFO_COUPLED_SINK_SUPPORT_AT_SOURCE      0x4
#define WFD_DEVICE_INFO_COUPLED_SINK_SUPPORT_AT_SINK        0x8
#define WFD_DEVICE_INFO_SESSION_AVAILABLE                   0x30
#define WFD_DEVICE_INFO_SESSION_AVAILABLE_BIT1              0x10
#define WFD_DEVICE_INFO_SESSION_AVAILABLE_BIT2              0x20

#define WPS_CONFIG_DISPLAY          0x0008
#define WPS_CONFIG_PUSHBUTTON       0x0080
#define WPS_CONFIG_KEYPAD           0x0100

class WfdDeviceInfo
{
public:
    WfdDeviceInfo() :
        dev_info_(0),
        ctrl_port_(0),
        max_tput_(0)
    {
    }

    WfdDeviceInfo(const WfdDeviceInfo &other) :
        dev_info_(other.dev_info_),
        ctrl_port_(other.ctrl_port_),
        max_tput_(other.max_tput_)
    {
    }

    WfdDeviceInfo(int dev_info, int ctrl_port, int max_tput) :
        dev_info_(dev_info),
        ctrl_port_(ctrl_port),
        max_tput_(max_tput)
    {
    }

    int DeviceType()
    {
        return (dev_info_ & WFD_DEVICE_INFO_DEVICE_TYPE);
    }

    int ControlPort()
    {
        return ctrl_port_;
    }

    int MaxThroughput()
    {
        return max_tput_;
    }

    bool IsSupportedSink()
    {
        return DeviceType() == WFD_DEVICE_TYPE_PRIMARY_SINK ||
               DeviceType() == WFD_DEVICE_TYPE_SOURCE_OR_PRIMARY_SINK;
    }

    std::string DeviceTypeAsString()
    {
        std::string str = "unknown";
        auto devType = DeviceType();

        if (devType == WFD_DEVICE_TYPE_SOURCE)
            str = "source";
        else if (devType == WFD_DEVICE_TYPE_PRIMARY_SINK)
            str = "primary-sink";
        else if (devType == WFD_DEVICE_TYPE_SECONDARY_SINK)
            str = "secondary-sink";
        else if (devType == WFD_DEVICE_TYPE_SOURCE_OR_PRIMARY_SINK)
            str = "source-or-primary-sink";

        return str;
    }

private:
    int dev_info_;
    int ctrl_port_;
    int max_tput_;
};

#endif
