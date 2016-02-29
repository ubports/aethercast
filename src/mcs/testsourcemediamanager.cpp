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

#include <gst/gst.h>

#include "logger.h"
#include "testsourcemediamanager.h"
#include "utils.h"
#include "logging.h"

#include "mcs/video/videoformat.h"

namespace mcs {
std::shared_ptr<TestSourceMediaManager> TestSourceMediaManager::create(const std::string &remote_address) {
    return std::shared_ptr<TestSourceMediaManager>{new TestSourceMediaManager{remote_address}};
}

TestSourceMediaManager::TestSourceMediaManager(const std::string &remote_address) :
    remote_address_(remote_address) {
}

TestSourceMediaManager::~TestSourceMediaManager() {
}

SharedGObject<GstElement> TestSourceMediaManager::ConstructPipeline(const wds::H264VideoFormat &format) {
    auto profile = mcs::video::ExtractH264Profile(format);
    auto rr = mcs::video::ExtractRateAndResolution(format);

#if 0
    std::stringstream ss;
    ss << Utils::Sprintf("videotestsrc ! video/x-raw,format=I420,framerate=%d/1,width=%d,height=%d,pixel-aspect-ratio=1/1 ! ", rr.framerate, rr.width, rr.height);
    ss << "x264enc tune=zerolatency byte-stream=true ! ";
    ss << Utils::Sprintf("video/x-h264,profile=%s ! ", profile.c_str());
    ss << "mpegtsmux ! rtpmp2tpay ! ";
    ss << Utils::Sprintf("udpsink name=sink host=%s port=%d", remote_address_.c_str(), sink_port1_);
#else
    std::stringstream ss;
    ss << "ximagesrc ! ";
    ss << Utils::Sprintf("videoconvert ! video/x-raw,format=I420,framerate=%d/1,pixel-aspect-ratio=1/1 ! ", rr.framerate);
    ss << Utils::Sprintf("videoscale ! video/x-raw,width=%d,height=%d ! ", rr.width, rr.height);
    ss << "queue2 ! ";
    ss << "x264enc byte-stream=true tune=zerolatency interlaced=false ! ";
    ss << Utils::Sprintf("video/x-h264,profile=%s !", profile.c_str());
    ss << "mpegtsmux ! rtpmp2tpay ! ";
    ss << Utils::Sprintf("udpsink name=sink host=%s port=%d", remote_address_.c_str(), sink_port1_);
#endif

    DEBUG("pipeline: %s", ss.str());

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(ss.str().c_str(), &error);
    if (error) {
        ERROR("Failed to setup GStreamer pipeline: %s", error->message);
        g_error_free(error);
        return nullptr;
    }

    return make_shared_gobject(pipeline);
}
} // namespace mcs
