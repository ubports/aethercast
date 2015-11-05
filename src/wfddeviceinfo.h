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

class WfdDevInfo
{
public:
    WfdDevInfo(int devInfo, int ctrlPort, int maxTput) :
        devInfo(devInfo),
        ctrlPort(ctrlPort),
        maxTput(maxTput)
    {
    }

    int deviceType()
    {
        return (devInfo & WFD_DEVICE_INFO_DEVICE_TYPE);
    }

    int controlPort()
    {
        return ctrlPort;
    }

    int maxThroughput()
    {
        return maxTput;
    }

    QString deviceTypeAsString()
    {
        QString str = "unknown";
        auto devType = deviceType();

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
    int devInfo;
    int ctrlPort;
    int maxTput;
};

#endif
