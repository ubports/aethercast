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

// Ignore all warnings coming from the external Android headers as
// we don't control them and also don't want to get any warnings
// from them which will only pollute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include <system/window.h>
#pragma GCC diagnostic pop

#include <boost/concept_check.hpp>

#include "ac/logger.h"
#include "ac/keep_alive.h"

#include "ac/android/h264encoder.h"

namespace {
static constexpr const char *kEncoderThreadName{"H264Encoder"};
static constexpr const char *kH264MimeType{"video/avc"};
static constexpr const char *kRawMimeType{"video/raw"};
// From frameworks/native/include/media/openmax/OMX_IVCommon.h
static constexpr int32_t kOMXColorFormatAndroidOpaque = 0x7F000789;
static constexpr int32_t kOMXVideoIntraRefreshCyclic = 0;
// From frameworks/native/include/media/openmax/OMX_Video.h
static constexpr int32_t kOMXVideoControlRateConstant = 2;
// Supplying -1 as framerate means the encoder decides on which framerate
// it provides.
static constexpr int32_t kAnyFramerate = -1;
// Default is a bitrate of 5 MBit/s
static constexpr int32_t kDefaultBitrate = 15000000;
// By default send an I frame every 15 seconds which is the
// same Android currently configures in its WiFi Display code path.
static constexpr std::chrono::seconds kDefaultIFrameInterval{15};
// From frameworks/av/include/media/stagefright/MediaErrors.h
enum AndroidMediaError {
    kAndroidMediaErrorBase = -1000,
    kAndroidMediaErrorNotConnected = kAndroidMediaErrorBase - 1,
    kAndroidMediaErrorBufferTooSmall = kAndroidMediaErrorBase - 9,
    kAndroidMediaErrorEndOfStream = kAndroidMediaErrorBase - 11,
};
// Constants for all the fields we're putting into the AMessage
// structure to configure the MediaCodec instance for our needs.
static constexpr const char *kFormatKeyMime{"mime"};
static constexpr const char *kFormatKeyStoreMetaDataInBuffers{"store-metadata-in-buffers"};
static constexpr const char *kFormatKeyStoreMetaDataInBuffers2{"android._input-metadata-buffer-type"};
static constexpr const char *kFormatKeyStoreMetaDataInBuffersOutput{"store-metadata-in-buffers-output"};
static constexpr const char *kFormatKeyStoreMetaDataInBuffersOutput2{"android._store-metadata-in-buffers-output"};
static constexpr const char *kFormatKeyUsingRecorder{"android._using-recorder"};
static constexpr const char *kFormatKeyWidth{"width"};
static constexpr const char *kFormatKeyHeight{"height"};
static constexpr const char *kFormatKeyStride{"stride"};
static constexpr const char *kFormatKeySliceHeight{"slice-height"};
static constexpr const char *kFormatKeyColorFormat{"color-format"};
static constexpr const char *kFormatKeyBitrate{"bitrate"};
static constexpr const char *kFormatKeyBitrateMode{"bitrate-mode"};
static constexpr const char *kFormatKeyFramerate{"frame-rate"};
static constexpr const char *kFormatKeyIntraRefreshMode{"intra-refresh-mode"};
static constexpr const char *kFormatKeyIntraRefreshCIRMbs{"intra-refresh-CIR-mbs"};
static constexpr const char *kFormatKeyIFrameInterval{"i-frame-interval"};
static constexpr const char *kFormatKeyProfileIdc{"profile-idc"};
static constexpr const char *kFormatKeyLevelIdc{"level-idc"};
static constexpr const char *kFormatKeyConstraintSet{"constraint-set"};
static constexpr const char *kFormatKeyPrependSpsPpstoIdrFrames{"prepend-sps-pps-to-idr-frames"};

typedef enum {
    kMetadataBufferTypeCameraSource  = 0,
    kMetadataBufferTypeGrallocSource = 1,
    kMetadataBufferTypeANWBuffer = 2,
    kMetadataBufferTypeNativeHandleSource = 3,
    kMetadataBufferTypeInvalid = -1,
} MetadataBufferType;

struct VideoNativeMetadata {
    MetadataBufferType eType;               // must be kMetadataBufferTypeANWBuffer
    ANativeWindowBuffer *pBuffer;
    int nFenceFd;                           // -1 if unused
};
}

namespace ac {
namespace android {

class MediaSourceBuffer : public video::Buffer
{
public:
    typedef std::shared_ptr<MediaSourceBuffer> Ptr;

    ~MediaSourceBuffer() {
        if (!buffer_)
            return;

        media_buffer_destroy(buffer_);
    }

    static MediaSourceBuffer::Ptr Create(MediaBufferWrapper *buffer) {
        const auto sp = std::shared_ptr<MediaSourceBuffer>(new MediaSourceBuffer);
        sp->buffer_ = buffer;
        sp->ExtractTimestamp();
        return sp;
    }

    virtual uint32_t Length() const {
        return media_buffer_get_size(buffer_);
    }

    virtual uint8_t* Data() {
        return static_cast<uint8_t*>(media_buffer_get_data(buffer_));
    }

    virtual bool IsValid() const {
        return buffer_ != nullptr;
    }

private:
    void ExtractTimestamp() {
        const auto meta_data = media_buffer_get_meta_data(buffer_);
        if (!meta_data)
            return;

        const uint32_t key_time = media_meta_data_get_key_id(MEDIA_META_DATA_KEY_TIME);
        int64_t time_us = 0;
        media_meta_data_find_int64(meta_data, key_time, &time_us);

        SetTimestamp(time_us);
    }

private:
    MediaBufferWrapper *buffer_;
};

video::BaseEncoder::Config H264Encoder::DefaultConfiguration() {
    Config config;
    config.framerate = kAnyFramerate;
    config.bitrate = kDefaultBitrate;
    config.i_frame_interval = kDefaultIFrameInterval.count();
    config.intra_refresh_mode = kOMXVideoIntraRefreshCyclic;
    return config;
}

video::BaseEncoder::Ptr H264Encoder::Create(const video::EncoderReport::Ptr &report) {
    return std::shared_ptr<H264Encoder>(new H264Encoder(report));
}

H264Encoder::H264Encoder(const video::EncoderReport::Ptr &report) :
    report_(report),
    format_(nullptr),
    source_format_(nullptr),
    encoder_(nullptr),
    running_(false),
    input_queue_(ac::video::BufferQueue::Create()),
    start_time_(-1ll),
    frame_count_(0) {
}

H264Encoder::~H264Encoder() {
    Stop();

    if (encoder_)
        media_codec_source_release(encoder_);

    if (format_)
        media_message_release(format_);

    if (source_format_)
        media_meta_data_release(source_format_);
}

bool H264Encoder::Configure(const Config &config) {
    if (encoder_)
        return false;

    AC_DEBUG("configuring with %dx%d@%d", config.width, config.height, config.framerate);

    auto format = media_message_create();
    if (!format)
        return false;

    media_message_set_string(format, kFormatKeyMime, kH264MimeType, 0);

    media_message_set_int32(format, kFormatKeyStoreMetaDataInBuffers, kMetadataBufferTypeANWBuffer);
    media_message_set_int32(format, kFormatKeyStoreMetaDataInBuffers2, kMetadataBufferTypeANWBuffer);
    media_message_set_int32(format, kFormatKeyStoreMetaDataInBuffersOutput, false);
    media_message_set_int32(format, kFormatKeyStoreMetaDataInBuffersOutput2, false);

    media_message_set_int32(format, kFormatKeyUsingRecorder, true);

    media_message_set_int32(format, kFormatKeyWidth, config.width);
    media_message_set_int32(format, kFormatKeyHeight, config.height);
    media_message_set_int32(format, kFormatKeyStride, config.width);
    media_message_set_int32(format, kFormatKeySliceHeight, config.width);

    media_message_set_int32(format, kFormatKeyColorFormat, kOMXColorFormatAndroidOpaque);

    media_message_set_int32(format, kFormatKeyBitrate, config.bitrate);
    media_message_set_int32(format, kFormatKeyBitrateMode, kOMXVideoControlRateConstant);
    media_message_set_int32(format, kFormatKeyFramerate, config.framerate);

    media_message_set_int32(format, kFormatKeyIntraRefreshMode, 0);

    // Update macroblocks in a cyclic fashion with 10% of all MBs within
    // frame gets updated at one time. It takes about 10 frames to
    // completely update a whole video frame. If the frame rate is 30,
    // it takes about 333 ms in the best case (if next frame is not an IDR)
    // to recover from a lost/corrupted packet.
    const int32_t mbs = (((config.width + 15) / 16) * ((config.height + 15) / 16) * 10) / 100;
    media_message_set_int32(format, kFormatKeyIntraRefreshCIRMbs, mbs);

    if (config.i_frame_interval > 0)
        media_message_set_int32(format, kFormatKeyIFrameInterval, config.i_frame_interval);

    if (config.profile_idc > 0)
        media_message_set_int32(format, kFormatKeyProfileIdc, config.profile_idc);

    if (config.level_idc > 0)
        media_message_set_int32(format, kFormatKeyLevelIdc, config.level_idc);

    if (config.constraint_set > 0)
        media_message_set_int32(format, kFormatKeyConstraintSet, config.constraint_set);

#if 0
    // FIXME we need to find a way to check if the encoder supports prepending
    // SPS/PPS to the buffers it is producing or if we have to manually do that
    media_message_set_int32(format, kFormatKeyPrependSpsPpstoIdrFrames, 1);
#endif

    auto source = media_source_create();
    if (!source) {
        AC_ERROR("Failed to create media input source for encoder");
        media_message_release(format);
        return false;
    }

    auto source_format = media_meta_data_create();
    if (!source_format) {
        AC_ERROR("Failed to create media meta data for encoder source");
        media_message_release(format);
        media_source_release(source);
        return false;
    }

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

    media_source_set_format(source, source_format);

    media_source_set_start_callback(source, &H264Encoder::OnSourceStart, this);
    media_source_set_stop_callback(source, &H264Encoder::OnSourceStop, this);
    media_source_set_read_callback(source, &H264Encoder::OnSourceRead, this);
    media_source_set_pause_callback(source, &H264Encoder::OnSourcePause, this);

    // The MediaSource will be now owned by the MediaCodecSource wrapper
    // inside our compatibility layer. It will make sure its freed when
    // needed.
    encoder_ = media_codec_source_create(format, source, 0);
    if (!encoder_) {
        AC_ERROR("Failed to create encoder instance");
        media_meta_data_release(source_format);
        media_message_release(format);
        media_source_release(source);
        return false;
    }

    config_ = config;
    format_ = format;
    source_format_ = source_format;

    AC_DEBUG("Configured encoder succesfully");

    return true;
}

bool H264Encoder::Start() {
    if (!encoder_ || running_)
        return false;

    // We have to set us to running before we start the media
    // codec source as that will directly call OnSourceRead
    // which will fail if running_ isn't set to true.
    running_ = true;

    if (!media_codec_source_start(encoder_)) {
        AC_ERROR("Failed to start encoder");
        running_ = false;
        return false;
    }

    report_->Started();

    return true;
}

int H264Encoder::OnSourceStart(MediaMetaDataWrapper *meta, void *user_data) {
    boost::ignore_unused_variable_warning(meta);
    boost::ignore_unused_variable_warning(user_data);

    AC_DEBUG("");

    return 0;
}

int H264Encoder::OnSourceStop(void *user_data) {
    boost::ignore_unused_variable_warning(user_data);

    AC_DEBUG("");

    return 0;
}

int H264Encoder::OnSourcePause(void *user_data) {
    boost::ignore_unused_variable_warning(user_data);

    AC_DEBUG("");

    return 0;
}

MediaBufferWrapper* H264Encoder::PackBuffer(const ac::video::Buffer::Ptr &input_buffer, const ac::TimestampUs &timestamp) {
    if (!input_buffer->NativeHandle()) {
        AC_WARNING("Ignoring buffer without native handle");
        return nullptr;
    }

    const auto anwb = reinterpret_cast<ANativeWindowBuffer*>(input_buffer->NativeHandle());

    // We let the media buffer allocate the memory here to let it keep
    // the ownership and release the memory once its destroyed.
    auto buffer = media_buffer_create(sizeof(VideoNativeMetadata));
    if (!buffer)
        return nullptr;

    VideoNativeMetadata* data = (VideoNativeMetadata*)media_buffer_get_data(buffer);

    memset(data, 0, sizeof(VideoNativeMetadata));
    data->eType = kMetadataBufferTypeANWBuffer;
    data->pBuffer = anwb;
    data->nFenceFd = -1;

    media_buffer_set_return_callback(buffer, &H264Encoder::OnBufferReturned, this);

#if 0
    // We need to put a reference on the buffer here if we want the
    // callback we set above being called.
    media_buffer_ref(buffer);
#endif

    auto meta = media_buffer_get_meta_data(buffer);
    const auto key_time = media_meta_data_get_key_id(MEDIA_META_DATA_KEY_TIME);
    media_meta_data_set_int64(meta, key_time, timestamp);

    pending_buffers_.push_back(BufferItem{input_buffer, buffer});

    return buffer;
}

int H264Encoder::OnSourceRead(MediaBufferWrapper **buffer, void *user_data) {
    auto thiz = static_cast<H264Encoder*>(user_data);

    if (!thiz || !thiz->running_)
        return kAndroidMediaErrorNotConnected;

    if (!buffer)
        return kAndroidMediaErrorBufferTooSmall;

    const auto input_buffer = thiz->input_queue_->Next();
    if (!input_buffer)
        return kAndroidMediaErrorEndOfStream;

    const auto next_buffer = thiz->PackBuffer(input_buffer, input_buffer->Timestamp());

    if (!next_buffer)
        return kAndroidMediaErrorEndOfStream;

    *buffer = next_buffer;

    thiz->report_->BeganFrame(input_buffer->Timestamp());

    return 0;
}

void H264Encoder::OnBufferReturned(MediaBufferWrapper *buffer, void *user_data) {
    auto thiz = static_cast<H264Encoder*>(user_data);

    if (!thiz)
        return;

    // Find the right pending buffer matching the returned one
    auto iter = thiz->pending_buffers_.begin();
    for (; iter != thiz->pending_buffers_.end(); ++iter) {
        if (iter->media_buffer == buffer)
            break;
    }

    if (iter == thiz->pending_buffers_.end()) {
        AC_WARNING("Didn't remember returned buffer!?");
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

bool H264Encoder::Execute() {
    if (!running_) {
        AC_ERROR("Tried to execute encoder while not started");
        return false;
    }

    MediaBufferWrapper *buffer = nullptr;
    if (!media_codec_source_read(encoder_, &buffer)) {
        AC_ERROR("Failed to read a new buffer from encoder");
        return false;
    }

    auto mbuf = MediaSourceBuffer::Create(buffer);

    report_->FinishedFrame(mbuf->Timestamp());

    if (DoesBufferContainCodecConfig(buffer)) {
        if (auto sp = delegate_.lock())
            sp->OnBufferWithCodecConfig(mbuf);
    }

    if (auto sp = delegate_.lock())
        sp->OnBufferAvailable(mbuf);

    return true;
}

bool H264Encoder::Stop() {
    if (!encoder_ || !running_)
        return false;

    if (!media_codec_source_stop(encoder_))
        return false;

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

video::BaseEncoder::Config H264Encoder::Configuration() const {
    return config_;
}

void H264Encoder::SendIDRFrame() {
    if (!encoder_)
        return;

    AC_DEBUG("");

    media_codec_source_request_idr_frame(encoder_);
}

std::string H264Encoder::Name() const {
    return kEncoderThreadName;
}

} // namespace android
} // namespace ac
