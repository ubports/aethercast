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

#include "mirsourcemediamanager.h"
#include "utilities.h"

MirSourceMediaManager::MirSourceMediaManager(const std::string &remote_address) :
    remote_address_(remote_address) {
}

MirSourceMediaManager::~MirSourceMediaManager() {
}

std::string MirSourceMediaManager::ConstructPipeline(const wds::H264VideoFormat &format) {
    int width = 0;
    int height = 0;

    switch (format.type) {
    case wds::CEA:
        switch (format.rate_resolution) {
        case wds::CEA640x480p60:
            width = 640;
            height = 480;
            break;
        case wds::CEA720x480p60:
        case wds::CEA720x480i60:
            width = 720;
            height = 480;
            break;
        case wds::CEA720x576p50:
        case wds::CEA720x576i50:
            width = 720;
            height = 576;
            break;
        case wds::CEA1280x720p30:
        case wds::CEA1280x720p60:
            width = 1280;
            height = 720;
            break;
        case wds::CEA1920x1080p30:
        case wds::CEA1920x1080p60:
        case wds::CEA1920x1080i60:
            width = 1920;
            height = 1080;
            break;
        case wds::CEA1280x720p25:
        case wds::CEA1280x720p50:
        case wds::CEA1280x720p24:
            width = 1280;
            height = 720;
            break;
        case wds::CEA1920x1080p25:
        case wds::CEA1920x1080p50:
        case wds::CEA1920x1080i50:
        case wds::CEA1920x1080p24:
            width = 1920;
            height = 1080;
            break;
        }

        break;
    default:
        break;
    }

    auto config = utilities::StringFormat("mirimagesrc mir-socket=/run/mir_socket ! videoconvert ! videoscale ! video/x-raw,width=%d,height=%d ! videoflip method=counterclockwise ! queue2 ! video/x-raw,format=I420 ! x264enc aud=false byte-stream=true tune=zerolatency ! mpegtsmux ! rtpmp2tpay ! udpsink name=sink host=%s port=%d",
                                     width, height, remote_address_.c_str(), sink_port1_);
    return config;
}
