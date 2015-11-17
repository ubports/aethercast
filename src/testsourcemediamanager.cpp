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

#include <gst/gst.h>

#include "testsourcemediamanager.h"
#include "utilities.h"

TestSourceMediaManager::TestSourceMediaManager(const std::string &remote_address) :
    remote_address_(remote_address) {
}

TestSourceMediaManager::~TestSourceMediaManager() {
}

GstElement* TestSourceMediaManager::ConstructPipeline(const wds::H264VideoFormat &format) {
    auto config = utilities::StringFormat("videotestsrc ! videoconvert ! video/x-raw,format=I420 ! x264enc ! mpegtsmux ! rtpmp2tpay ! udpsink name=sink host=%s port=%d",
                                     remote_address_.c_str(), sink_port1_);

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(config.c_str(), &error);
    if (error) {
        g_warning("Failed to setup GStreamer pipeline: %s", error->message);
        g_error_free(error);
        return nullptr;
    }

    return pipeline;
}
