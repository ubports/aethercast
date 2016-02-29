/*
 * Copyright (C) 2016 Canonical, Ltd.
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

#include <system/window.h>

#include "mcs/logger.h"

#include "mcs/video/statistics.h"

#include "mcs/android/h264encoder.h"

namespace {
static const char *kEncoderThreadName{"H264Encoder"};
static const char *kH264MimeType{"video/avc"};
static const char *kRawMimeType{"video/raw"};
// From frameworks/native/include/media/openmax/OMX_IVCommon.h
const int32_t kOMXColorFormatAndroidOpaque = 0x7F000789;
const int32_t kOMXVideoIntraRefreshCyclic = 0;
// From frameworks/native/include/media/openmax/OMX_Video.h
const int32_t kOMXVideoControlRateConstant = 2;
// From frameworks/native/include/media/hardware/MetadataBufferType.h
const uint32_t kMetadataBufferTypeGrallocSource = 1;
// From frameworks/av/include/media/stagefright/MediaErrors.h
enum AndroidMediaError {
    kAndroidMediaErrorBase = -1000,
    kAndroidMediaErrorEndOfStream = kAndroidMediaErrorBase - 11,
};
}

namespace mcs {
namespace android {

class MediaSourceBuffer : public video::Buffer
{
public:
    typedef std::shared_ptr<MediaSourceBuffer> Ptr;

    ~MediaSourceBuffer() {
        if (!buffer_)
            return;

        auto ref_count = media_buffer_get_refcount(buffer_);

        // If someone has set a reference on the buffer we just have to
        // release it here and the other one will take care about actually
        // destroying it.
        if (ref_count > 0)
            media_buffer_release(buffer_);
        else
            media_buffer_destroy(buffer_);

    }

    static MediaSourceBuffer::Ptr Create(MediaBufferWrapper *buffer) {
        auto sp = std::shared_ptr<MediaSourceBuffer>(new MediaSourceBuffer);
        sp->buffer_ = buffer;
        sp->ExtractTimestamp();
        return sp;
    }

    virtual uint32_t Length() const {
        if (!buffer_)
            return 0;

        return media_buffer_get_size(buffer_);
    }

    virtual uint8_t* Data() {
        if (!buffer_)
            return nullptr;

        return static_cast<uint8_t*>(media_buffer_get_data(buffer_));
    }

    virtual bool IsValid() const {
        return buffer_ != nullptr;
    }

private:
    MediaSourceBuffer() { }

    void ExtractTimestamp() {
        auto meta_data = media_buffer_get_meta_data(buffer_);
        if (!meta_data)
            return;

        uint32_t key_time = media_meta_data_get_key_id(MEDIA_META_DATA_KEY_TIME);
        int64_t time_us = 0;
        media_meta_data_find_int64(meta_data, key_time, &time_us);

        SetTimestamp(time_us);
    }

private:
    MediaBufferWrapper *buffer_;
};

video::BaseEncoder::Config H264Encoder::DefaultConfiguration() {
    Config config;
    // Supplying -1 as framerate means the encoder decides on what it
    // can provide.
    config.framerate = -1;
    config.bitrate = 5000000;
    config.i_frame_interval = 15;
    config.intra_refresh_mode = kOMXVideoIntraRefreshCyclic;
    return config;
}

H264Encoder::Ptr H264Encoder::Create() {
    return std::shared_ptr<H264Encoder>(new H264Encoder);
}

H264Encoder::H264Encoder() :
    format_(nullptr),
    encoder_(nullptr),
    running_(false),
    input_queue_(mcs::video::BufferQueue::Create()),
    start_time_(-1ll),
    frame_count_(0) {
}

H264Encoder::~H264Encoder() {
    Stop();

    if (encoder_)
        media_codec_source_release(encoder_);

    if (source_)
        media_source_release(source_);

    if (format_)
        media_message_release(format_);
}

bool H264Encoder::IsValid() const {
    return true;
}

bool H264Encoder::Configure(const Config &config) {
    if (encoder_)
        return false;

    MCS_DEBUG("configuring with %dx%d@%d", config.width, config.height, config.framerate);

    auto format = media_message_create();
    if (!format)
        return false;

    media_message_set_string(format, "mime", kH264MimeType, 0);

    media_message_set_int32(format, "store-metadata-in-buffers", true);
    media_message_set_int32(format, "store-metadata-in-buffers-output", false);

    media_message_set_int32(format, "width", config.width);
    media_message_set_int32(format, "height", config.height);
    media_message_set_int32(format, "stride", config.width);
    media_message_set_int32(format, "slice-height", config.width);

    media_message_set_int32(format, "color-format", kOMXColorFormatAndroidOpaque);

    media_message_set_int32(format, "bitrate", config.bitrate);
    media_message_set_int32(format, "bitrate-mode", kOMXVideoControlRateConstant);
    media_message_set_int32(format, "frame-rate", config.framerate);

    media_message_set_int32(format, "intra-refresh-mode", 0);

    // Update macroblocks in a cyclic fashion with 10% of all MBs within
    // frame gets updated at one time. It takes about 10 frames to
    // completely update a whole video frame. If the frame rate is 30,
    // it takes about 333 ms in the best case (if next frame is not an IDR)
    // to recover from a lost/corrupted packet.
    int32_t mbs = (((config.width + 15) / 16) * ((config.height + 15) / 16) * 10) / 100;
    media_message_set_int32(format, "intra-refresh-CIR-mbs", mbs);

    if (config.i_frame_interval > 0)
        media_message_set_int32(format, "i-frame-interval", config.i_frame_interval);

    if (config.profile_idc > 0)
        media_message_set_int32(format, "profile-idc", config.profile_idc);

    if (config.level_idc > 0)
        media_message_set_int32(format, "level-idc", config.level_idc);

    if (config.constraint_set > 0)
        media_message_set_int32(format, "constraint-set", config.constraint_set);

    // FIXME we need to find a way to check if the encoder supports prepending
    // SPS/PPS to the buffers it is producing or if we have to manually do that
    media_message_set_int32(format, "prepend-sps-pps-to-idr-frames", 1);

    source_ = media_source_create();
    if (!source_) {
        MCS_ERROR("Failed to create media input source for encoder");
        media_message_release(format);
        return false;
    }

    auto source_format = media_meta_data_create();

    // Notice that we're passing video/raw as mime type here which is quite
    // important to let the encoder do the right thing with the incoming data
    media_meta_data_set_cstring(source_format,
        media_meta_data_get_key_id(MEDIA_META_DATA_KEY_MIME),
        kRawMimeType);

    // We're setting the opaque color format here as the encoder is then
    // meant to figure out the color format from the GL frames itself.
    media_meta_data_set_int32(source_format,
        media_meta_data_get_key_id(MEDIA_META_DATA_KEY_COLOR_FORMAT),
        kOMXColorFormatAndroidOpaque);

    media_meta_data_set_int32(source_format,
        media_meta_data_get_key_id(MEDIA_META_DATA_KEY_WIDTH),
        config.width);
    media_meta_data_set_int32(source_format,
        media_meta_data_get_key_id(MEDIA_META_DATA_KEY_HEIGHT),
        config.height);
    media_meta_data_set_int32(source_format,
        media_meta_data_get_key_id(MEDIA_META_DATA_KEY_STRIDE),
        config.width);
    media_meta_data_set_int32(source_format,
        media_meta_data_get_key_id(MEDIA_META_DATA_KEY_SLICE_HEIGHT),
        config.height);
    media_meta_data_set_int32(source_format,
        media_meta_data_get_key_id(MEDIA_META_DATA_KEY_FRAMERATE),
        config.framerate);

    media_source_set_format(source_, source_format);

    media_source_set_start_callback(source_, &H264Encoder::OnSourceStart, this);
    media_source_set_stop_callback(source_, &H264Encoder::OnSourceStop, this);
    media_source_set_read_callback(source_, &H264Encoder::OnSourceRead, this);
    media_source_set_pause_callback(source_, &H264Encoder::OnSourcePause, this);

    encoder_ = media_codec_source_create(format, source_, 0);
    if (!encoder_) {
        MCS_ERROR("Failed to create encoder instance");
        return false;
    }

    config_ = config;
    format_ = format;

    MCS_DEBUG("Configured encoder succesfully");

    return true;
}

void H264Encoder::Start() {
    if (!encoder_ || running_)
        return;

    if (!media_codec_source_start(encoder_)) {
        MCS_ERROR("Failed to start encoder");
        return;
    }

    worker_thread_ = std::thread(&H264Encoder::WorkerThread, this);

    running_ = true;

    MCS_DEBUG("Started encoder");
}

int H264Encoder::OnSourceStart(MediaMetaDataWrapper *meta, void *user_data) {
    auto thiz = static_cast<H264Encoder*>(user_data);

    MCS_DEBUG("");

    return 0;
}

int H264Encoder::OnSourceStop(void *user_data) {
    auto thiz = static_cast<H264Encoder*>(user_data);

    MCS_DEBUG("");

    return 0;
}

MediaBufferWrapper* H264Encoder::PackBuffer(const mcs::video::Buffer::Ptr &input_buffer, const mcs::TimestampUs &timestamp) {
    if (!input_buffer->NativeHandle()) {
        MCS_WARNING("Ignoring buffer without native handle");
        return nullptr;
    }

    auto anwb = reinterpret_cast<ANativeWindowBuffer*>(input_buffer->NativeHandle());

    size_t size = sizeof(buffer_handle_t) + 4;

    // We let the media buffer allocate the memory here to let it keep
    // the ownership and release the memory once its destroyed.
    auto buffer = media_buffer_create(size);
    auto data = media_buffer_get_data(buffer);

    // We're passing the buffer handle directly as part of the buffer data
    // here to the encoder and it will figure out it has to deal with a
    // buffer with the key value we put in front of it. See also
    // frameworks/av/media/libstagefright/SurfaceMediaSource.cpp for more
    // details about this.
    uint32_t type = kMetadataBufferTypeGrallocSource;
    memcpy(data, &type, 4);
    memcpy(data + 4, &anwb->handle, sizeof(buffer_handle_t));

    media_buffer_set_return_callback(buffer, &H264Encoder::OnBufferReturned, this);

    // We need to put a reference on the buffer here if we want the
    // callback we set above being called.
    media_buffer_ref(buffer);

    auto meta = media_buffer_get_meta_data(buffer);
    auto key_time = media_meta_data_get_key_id(MEDIA_META_DATA_KEY_TIME);
    media_meta_data_set_int64(meta, key_time, timestamp);

    pending_buffers_.push_back(BufferItem{input_buffer, buffer});

    return buffer;
}

int H264Encoder::OnSourceRead(MediaBufferWrapper **buffer, void *user_data) {
    auto thiz = static_cast<H264Encoder*>(user_data);

    auto input_buffer = thiz->input_queue_->Next();


    auto next_buffer = thiz->PackBuffer(input_buffer, input_buffer->Timestamp());

    if (!next_buffer)
        return kAndroidMediaErrorEndOfStream;

    *buffer = next_buffer;

    return 0;
}

int H264Encoder::OnSourcePause(void *user_data) {
    auto thiz = static_cast<H264Encoder*>(user_data);

    MCS_DEBUG("");

    return 0;
}

void H264Encoder::OnBufferReturned(MediaBufferWrapper *buffer, void *user_data) {
    auto thiz = static_cast<H264Encoder*>(user_data);

    // Find the right pending buffer matching the returned one
    auto iter = thiz->pending_buffers_.begin();
    for (; iter != thiz->pending_buffers_.end(); ++iter) {
        if (iter->media_buffer == buffer)
            break;
    }

    if (iter == thiz->pending_buffers_.end()) {
        MCS_WARNING("Didn't remember returned buffer!?");
        return;
    }

    // Unset observer to be able to call release on the MediaBuffer
    // and reduce its reference count. It has an internal check if
    // an observer is still set or not before it will actually release
    // itself.
    media_buffer_set_return_callback(iter->media_buffer, nullptr, nullptr);
    media_buffer_release(iter->media_buffer);

    auto buf = iter->buffer;
    thiz->pending_buffers_.erase(iter);

    // After we've cleaned up everything we can send the buffer
    // back to the producer which then can reuse it.
    buf->Release();
}

bool H264Encoder::DoesBufferContainCodecConfig(MediaBufferWrapper *buffer) {
    auto meta_data = media_buffer_get_meta_data(buffer);
    if (!meta_data)
        return false;

    uint32_t key_is_codec_config = media_meta_data_get_key_id(MEDIA_META_DATA_KEY_IS_CODEC_CONFIG);
    int32_t is_codec_config = 0;
    media_meta_data_find_int32(meta_data, key_is_codec_config, &is_codec_config);
    return static_cast<bool>(is_codec_config);
}

void H264Encoder::WorkerThread() {
    mcs::Utils::SetThreadName(kEncoderThreadName);

    MCS_DEBUG("Encoder worker thread is running");

    while (running_) {
        MediaBufferWrapper *buffer = nullptr;
        if (!media_codec_source_read(encoder_, &buffer)) {
            MCS_ERROR("Failed to read a new buffer from encoder");
            return;
        }

        auto mbuf = MediaSourceBuffer::Create(buffer);

        if (mbuf->Timestamp() > 0) {
            int64_t now = mcs::Utils::GetNowUs();
            int64_t diff = (now - mbuf->Timestamp()) / 1000ll;
            video::Statistics::Instance()->RecordEncoderBufferOut(diff);
        }

        if (DoesBufferContainCodecConfig(buffer)) {
            if (auto sp = delegate_.lock())
                sp->OnBufferWithCodecConfig(mbuf);
        }

        if (auto sp = delegate_.lock())
            sp->OnBufferAvailable(mbuf);
    }
}

void H264Encoder::Stop() {
    if (!encoder_ || !running_)
        return;

    if (!media_codec_source_stop(encoder_))
        return;

    running_ = false;
    worker_thread_.join();
}

void H264Encoder::QueueBuffer(const video::Buffer::Ptr &buffer) {
    input_queue_->Push(buffer);
}

void* H264Encoder::NativeWindowHandle() const {
    if (!encoder_)
        return nullptr;

    return media_codec_source_get_native_window_handle(encoder_);
}

video::BaseEncoder::Config H264Encoder::Configuration() const {
    return config_;
}

void H264Encoder::SendIDRFrame() {
    if (!encoder_)
        return;

    MCS_DEBUG("");

    media_codec_source_request_idr_frame(encoder_);
}

} // namespace android
} // namespace mcs
