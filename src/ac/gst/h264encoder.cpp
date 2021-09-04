/*
 * Copyright (C) 2016 Canonical, Ltd.
 * Copyright (C) 2021 Alfred Neumayer
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

// Ignore all warnings coming from the external Android headers as
// we don't control them and also don't want to get any warnings
// from them which will only pollute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include <system/window.h>
#pragma GCC diagnostic pop

#include <cstring>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include <boost/concept_check.hpp>

#include "ac/logger.h"
#include "ac/keep_alive.h"

#include "ac/gst/h264encoder.h"

namespace {
static constexpr const char *kEncoderThreadName{"GstH264Encoder"};
static constexpr const char *kH264MimeType{"video/x-h264"};
static constexpr const char *kRawMimeType{"video/x-raw"};
// Supplying -1 as framerate means the encoder decides on which framerate
// it provides.
static constexpr int32_t kAnyFramerate = 30;
// Default is a bitrate of 5 MBit/s
static constexpr int32_t kDefaultBitrate = 5000000;
// By default send an I frame every 15 seconds which is the
// same Android currently configures in its WiFi Display code path.
static constexpr std::chrono::seconds kDefaultIFrameInterval{15};
}

namespace ac {
namespace gst {

class GstH264Buffer : public video::Buffer
{
public:
    typedef std::shared_ptr<GstH264Buffer> Ptr;

    ~GstH264Buffer() {
        if (!buffer_)
            return;
        gst_buffer_unmap(buffer_, &mapinfo_);
        gst_buffer_unref(buffer_);
    }

    static GstH264Buffer::Ptr Create(GstBuffer *buffer) {
        const auto sp = std::shared_ptr<GstH264Buffer>(new GstH264Buffer);
        sp->buffer_ = buffer;
        sp->ExtractTimestamp();

        gst_buffer_map(sp->buffer_, &sp->mapinfo_, GST_MAP_READ);
        return sp;
    }

    virtual uint32_t Length() const {
        return gst_buffer_get_size(buffer_);
    }

    virtual uint8_t* Data() {
        return static_cast<uint8_t*>(mapinfo_.data);
    }

    virtual bool IsValid() const {
        return buffer_ != nullptr;
    }

private:
    void ExtractTimestamp() {
        if (!buffer_)
            return;

        SetTimestamp(buffer_->dts);
    }

private:
    GstMapInfo mapinfo_;
    GstBuffer *buffer_;
};

static void start_feed(GstElement *pipeline, guint size, H264Encoder *data) {
    static bool isApplied = false;
    if (!isApplied) {
        data->ApplyConfiguration();
        isApplied = true;
    }
    data->PushBuffer();
}

static GstFlowReturn new_sample(GstElement* sink, H264Encoder *data) {
    // Fetch encoded frame
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample) {
        AC_DEBUG("No encoded sample received");
        return GST_FLOW_EOS;
    }

    GstBuffer* output_buffer = gst_sample_get_buffer(sample);
    if (!output_buffer) {
        gst_sample_unref(sample);
        AC_WARNING("Failed to get output buffer");
        return GST_FLOW_EOS;
    }

    // Create sendable video buffer out of gstreamer output buffer
    video::Buffer::Ptr send_buffer = GstH264Buffer::Create(output_buffer);

    if (auto sp = data->Delegate().lock())
        sp->OnBufferAvailable(send_buffer);

    data->Report()->FinishedFrame(send_buffer->Timestamp());

    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

video::BaseEncoder::Config H264Encoder::DefaultConfiguration() {
    Config config;
    config.framerate = kAnyFramerate;
    config.bitrate = kDefaultBitrate;
    config.i_frame_interval = kDefaultIFrameInterval.count();
    return config;
}

video::BaseEncoder::Ptr H264Encoder::Create(const video::EncoderReport::Ptr &report) {
    return std::shared_ptr<H264Encoder>(new H264Encoder(report));
}

H264Encoder::H264Encoder(const video::EncoderReport::Ptr &report) :
    report_(report),
    running_(false),
    input_queue_(ac::video::BufferQueue::Create()),
    start_time_(-1ll),
    frame_count_(0)
{
    // We don't need cmdline argument parsing
    gst_init(nullptr, nullptr);

    context_ = g_main_context_new();
    if (!context_) {
        AC_WARNING("Failed to set up main context");
        return;
    }

    loop_ =  g_main_loop_new (context_, false);
    if (!loop_) {
        AC_WARNING("Failed to set up main loop");
        return;
    }

    appsource_ = gst_element_factory_make("appsrc", nullptr);
    if (!appsource_) {
        AC_WARNING("Failed to set up appsink");
        return;
    }
    g_object_set(G_OBJECT(appsource_),
        "stream-type", 0,
        "is-live", TRUE,
        "format", GST_FORMAT_TIME,
        nullptr);

    converter_ = gst_element_factory_make("videoconvert", nullptr);
    if (!converter_) {
        AC_WARNING("Failed to set up converter");
        return;
    }

    encoder_ = gst_element_factory_make("droidvenc", nullptr);
    if (!encoder_) {
        AC_WARNING("Failed to set up encoder");
        return;
    }

    appsink_ = gst_element_factory_make("appsink", nullptr);
    if (!appsink_) {
        AC_WARNING("Failed to set up appsink");
        return;
    }

    pipeline_ = gst_pipeline_new(nullptr);
    if (!pipeline_) {
        AC_WARNING("Failed to set up pipeline");
        return;
    }

    gst_bin_add_many(GST_BIN(pipeline_), appsource_, converter_, encoder_, appsink_, NULL);
    gst_element_link_many(appsource_, converter_, encoder_, appsink_, NULL);

    /* Connect to the pad-added signal */
    g_signal_connect(appsource_, "need-data", G_CALLBACK(start_feed), this);
    g_signal_connect(appsink_, "new-sample", G_CALLBACK(new_sample), this);

    AC_DEBUG("GstH264Encoder created");
}

H264Encoder::~H264Encoder() {
    Stop();

    g_main_loop_unref (loop_);

    if (pipeline_) {
        gst_object_unref(pipeline_);
    } else {
        if (converter_)
            gst_object_unref(converter_);
        if (encoder_)
            gst_object_unref(encoder_);
        if (appsink_)
            gst_object_unref(appsink_);
        if (appsource_)
            gst_object_unref(appsource_);
    }

    gst_deinit();
}

bool H264Encoder::Configure(const Config &config) {
    if (!encoder_)
        return false;

    AC_DEBUG("configuring with %dx%d@%d", config.width, config.height, config.framerate);

    config_ = config;

    AC_DEBUG("Configured encoder succesfully");

    return true;
}

bool H264Encoder::ApplyConfiguration() {
    GstCaps *caps;

    AC_DEBUG("Applying encoder configuration");

    // Set caps for the converter
    /*caps = gst_caps_new_simple(
        kRawMimeType, "format", G_TYPE_STRING, "YV12",
        "width", G_TYPE_INT, config_.width,
        "height", G_TYPE_INT, config_.height,
        "framerate", GST_TYPE_FRACTION, config_.framerate, 1, NULL);
    gst_caps_set_features(caps, 0, gst_caps_features_new("memory:DroidVideoMetaData", NULL));
    if (!gst_element_link_filtered(converter_, encoder_, caps)) {
        AC_WARNING("Failed to link (filtered) encoder");
        return false;
    }
    gst_caps_unref(caps);

    // Then set caps for sink
    caps = gst_caps_new_simple(
        kH264MimeType, "format", G_TYPE_STRING, "YV12",
        "width", G_TYPE_INT, config_.width,
        "height", G_TYPE_INT, config_.height,
        "framerate", GST_TYPE_FRACTION, config_.framerate, 1, NULL);

    //g_object_set(G_OBJECT(appsink_), "caps", caps, NULL);
    if (!gst_element_link_filtered(encoder_, appsink_, caps)) {
        AC_WARNING("Failed to link (filtered) appsink");
        return false;
    }
    gst_caps_unref(caps);*/
}

bool H264Encoder::Start() {
    if (!encoder_ || !pipeline_ || running_)
        return false;

    running_ = true;

    GstStateChangeReturn state_change = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    AC_DEBUG("state change: %d", state_change);
    if (state_change == GST_STATE_CHANGE_FAILURE) {
        AC_ERROR("Couldn't set pipeline to playback");
        return false;
    }

    report_->Started();

    return true;
}

bool H264Encoder::Execute() {
    if (!running_) {
        AC_ERROR("Tried to execute encoder while not started");
        return false;
    }

    g_main_loop_run(loop_);

    return false;
}

bool H264Encoder::Stop() {
    if (!encoder_ || !pipeline_ || !running_)
        return false;

    if (loop_) {
        gst_element_send_event(pipeline_, gst_event_new_eos());
        g_main_loop_quit(loop_);
    }

    running_ = false;

    report_->Stopped();

    return true;
}

void H264Encoder::QueueBuffer(const video::Buffer::Ptr &buffer) {
    if (!running_)
        return;

    input_queue_->Push(buffer);

    report_->ReceivedInputBuffer(buffer->Timestamp());
}

void H264Encoder::PushBuffer() {
    GstMapInfo map;

    // Get new frame from Mir first
    AC_DEBUG("Getting next frame from input queue");
    video::Buffer::Ptr input_buffer = input_queue_->Next();

    report_->BeganFrame(input_buffer->Timestamp());

    // Feed frame into GStreamer for encoding with our prefered encoder
    GstBuffer* feed_buffer = gst_buffer_new_and_alloc (input_buffer->Length());
    gst_buffer_map(feed_buffer, &map, GST_MAP_WRITE);
    std::memcpy(map.data, input_buffer->Data(), input_buffer->Length());
    gst_buffer_unmap(feed_buffer, &map);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsource_), feed_buffer);
    if (ret != GST_FLOW_OK) {
        AC_DEBUG("Flow not okay, trying next iteration");
        return;
    }
}

video::BaseEncoder::Config H264Encoder::Configuration() const {
    return config_;
}

std::weak_ptr<ac::video::BaseEncoder::Delegate> H264Encoder::Delegate() {
    return delegate_;
}

video::EncoderReport::Ptr H264Encoder::Report() {
    return report_;
}

void H264Encoder::SendIDRFrame() {
    // Not supported for now
}

std::string H264Encoder::Name() const {
    return kEncoderThreadName;
}

} // namespace gst
} // namespace ac
