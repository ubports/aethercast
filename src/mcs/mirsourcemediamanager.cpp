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

#include <sstream>

#include "mirsourcemediamanager.h"
#include "utils.h"
#include "logging.h"

namespace mcs {
MirSourceMediaManager::MirSourceMediaManager(const std::string &remote_address) :
    remote_address_(remote_address) {
}

MirSourceMediaManager::~MirSourceMediaManager() {
    g_warning("MirSourceMediaManager::~MirSourceMediaManager");
}

SharedGObject<GstElement> MirSourceMediaManager::ConstructPipeline(const wds::H264VideoFormat &format) {
    int width = 0, height = 0;
    std::string profile = "constrained-baseline";

    switch (format.profile) {
    case wds::CBP:
        profile = "constrained-baseline";
        break;
    case wds::CHP:
        profile = "high";
        break;
    }

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
        default:
            break;
        }
        break;
    default:
        break;
    }

    std::stringstream ss;
    ss << "mirimagesrc mir-socket=/run/mir_socket ! videoconvert ! videoscale ! ";
    ss << Utils::Sprintf("video/x-raw,width=%d,height=%d ! ", width, height);
    ss << "videoflip method=counterclockwise ! queue2 ! video/x-raw,format=I420 ! ";
    ss << "x264enc aud=false byte-stream=true tune=zerolatency ! ";
    ss << Utils::Sprintf("video/x-h264,profile=%s ! ", profile.c_str());
    ss << "mpegtsmux ! rtpmp2tpay ! ";
    ss << Utils::Sprintf("udpsink name=sink host=%s port=%d", remote_address_.c_str(), sink_port1_);

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(ss.str().c_str(), &error);
    if (error) {
        mcs::Error("Failed to setup GStreamer pipeline: %s", error->message);
        g_error_free(error);
        return nullptr;
    }

    return make_shared_gobject(pipeline);
}
} // namespace mcs
