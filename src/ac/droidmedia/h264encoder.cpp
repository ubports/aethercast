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

#include <unistd.h>
#include <chrono>
#include <cstring>

#include <boost/concept_check.hpp>

#include "ac/logger.h"
#include "ac/keep_alive.h"

#include "ac/droidmedia/h264encoder.h"

namespace {


typedef enum {
    /*
     * kMetadataBufferTypeCameraSource is used to indicate that
     * the source of the metadata buffer is the camera component.
     */
    kMetadataBufferTypeCameraSource  = 0,
    /*
     * kMetadataBufferTypeGrallocSource is used to indicate that
     * the payload of the metadata buffers can be interpreted as
     * a buffer_handle_t.
     * So in this case,the metadata that the encoder receives
     * will have a byte stream that consists of two parts:
     * 1. First, there is an integer indicating that it is a GRAlloc
     * source (kMetadataBufferTypeGrallocSource)
     * 2. This is followed by the buffer_handle_t that is a handle to the
     * GRalloc buffer. The encoder needs to interpret this GRalloc handle
     * and encode the frames.
     * --------------------------------------------------------------
     * |  kMetadataBufferTypeGrallocSource | buffer_handle_t buffer |
     * --------------------------------------------------------------
     *
     * See the VideoGrallocMetadata structure.
     */
    kMetadataBufferTypeGrallocSource = 1,
    /*
     * kMetadataBufferTypeGraphicBuffer is used to indicate that
     * the payload of the metadata buffers can be interpreted as
     * an ANativeWindowBuffer, and that a fence is provided.
     *
     * In this case, the metadata will have a byte stream that consists of three parts:
     * 1. First, there is an integer indicating that the metadata
     * contains an ANativeWindowBuffer (kMetadataBufferTypeANWBuffer)
     * 2. This is followed by the pointer to the ANativeWindowBuffer.
     * Codec must not free this buffer as it does not actually own this buffer.
     * 3. Finally, there is an integer containing a fence file descriptor.
     * The codec must wait on the fence before encoding or decoding into this
     * buffer. When the buffer is returned, codec must replace this file descriptor
     * with a new fence, that will be waited on before the buffer is replaced
     * (encoder) or read (decoder).
     * ---------------------------------
     * |  kMetadataBufferTypeANWBuffer |
     * ---------------------------------
     * |  ANativeWindowBuffer *buffer  |
     * ---------------------------------
     * |  int fenceFd                  |
     * ---------------------------------
     *
     * See the VideoNativeMetadata structure.
     */
    kMetadataBufferTypeANWBuffer = 2,
    /*
     * kMetadataBufferTypeNativeHandleSource is used to indicate that
     * the payload of the metadata buffers can be interpreted as
     * a native_handle_t.
     *
     * In this case, the metadata that the encoder receives
     * will have a byte stream that consists of two parts:
     * 1. First, there is an integer indicating that the metadata contains a
     * native handle (kMetadataBufferTypeNativeHandleSource).
     * 2. This is followed by a pointer to native_handle_t. The encoder needs
     * to interpret this native handle and encode the frame. The encoder must
     * not free this native handle as it does not actually own this native
     * handle. The handle will be freed after the encoder releases the buffer
     * back to camera.
     * ----------------------------------------------------------------
     * |  kMetadataBufferTypeNativeHandleSource | native_handle_t* nh |
     * ----------------------------------------------------------------
     *
     * See the VideoNativeHandleMetadata structure.
     */
    kMetadataBufferTypeNativeHandleSource = 3,
    /* This value is used by framework, but is never used inside a metadata buffer  */
    kMetadataBufferTypeInvalid = -1,
    // Add more here...
} MetadataBufferType;

struct VideoNativeMetadata {
    MetadataBufferType eType;               // must be kMetadataBufferTypeANWBuffer
    ANativeWindowBuffer *pBuffer;
    int nFenceFd;                           // -1 if unused
};

static constexpr const char *kEncoderThreadName{"DroidmediaH264Encoder"};
static constexpr const char *kH264MimeType{"video/avc"};
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
namespace droidmedia {

class DroidmediaH264Buffer : public video::Buffer
{
public:
    typedef std::shared_ptr<DroidmediaH264Buffer> Ptr;

    ~DroidmediaH264Buffer() {
        if (!buffer_)
            return;

        buffer_ = nullptr;
    }

    static DroidmediaH264Buffer::Ptr Create(DroidMediaCodecData *buffer) {
        const auto sp = std::shared_ptr<DroidmediaH264Buffer>(new DroidmediaH264Buffer);
        sp->buffer_ = buffer;
        sp->ExtractTimestamp();
        return sp;
    }

    virtual uint32_t Length() const {
        return buffer_->data.size;
    }

    virtual uint8_t* Data() {
        return static_cast<uint8_t*>(buffer_->data.data);
    }

    virtual bool IsValid() const {
        return buffer_ != nullptr;
    }

private:
    void ExtractTimestamp() {
        if (!buffer_)
            return;

        SetTimestamp(buffer_->ts);
    }
private:
    DroidMediaCodecData* buffer_;
};

static void signal_eos(void *data) {
    H264Encoder *encoder = static_cast<H264Encoder *>(data);
    encoder->Stop();
}

static void signal_error(void *data, int err) {
    H264Encoder *encoder = static_cast<H264Encoder *>(data);

    AC_ERROR("Error occured during encoding, stopping: %d", err);
    encoder->Stop();
}

static void signal_release(void *data)
{
    AC_DEBUG("Release");
    //delete[] data;
    DroidMediaBuffer* buffer = static_cast<DroidMediaBuffer*>(data);
    droid_media_buffer_unlock(buffer);
    droid_media_buffer_destroy(buffer);
}

static void signal_release_readout(void *data)
{
    AC_DEBUG("Release pixels");
    char* buffer = static_cast<char*>(data);
    delete[] buffer;
}

static void data_available(void *data, DroidMediaCodecData *encoded)
{
    H264Encoder *encoder = static_cast<H264Encoder *>(data);
    AC_DEBUG("Data available, processing");

    // Create sendable video buffer out of output buffer
    video::Buffer::Ptr send_buffer = DroidmediaH264Buffer::Create(encoded);
    AC_DEBUG("send_buffer length: %lu", (unsigned long) send_buffer->Length());

    /*if (encoded) {
        if (auto sp = encoder->Delegate().lock())
            sp->OnBufferWithCodecConfig(mbuf);
    }*/

    if (auto sp = encoder->Delegate().lock())
        sp->OnBufferAvailable(send_buffer);

    encoder->Report()->FinishedFrame(send_buffer->Timestamp());
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
    //droid_media_init();
    AC_DEBUG("DroidmediaH264Encoder created");
}

H264Encoder::~H264Encoder() {
    Stop();

    if (codec_) {
        droid_media_codec_destroy(codec_);
        codec_ = nullptr;
    }

    //droid_media_deinit();
}

bool H264Encoder::Configure(const Config &config) {
    AC_DEBUG("configuring with %dx%d@%d", config.width, config.height, config.framerate);

    const size_t maxPixels = ((config.width + 15) / 16) * ((config.height + 15) / 16) * 16 * 16;
    const int minCompressionRatio = 2;
    const size_t maxInputSize = (maxPixels * 3) / (2 * minCompressionRatio);

    DroidMediaCodecEncoderMetaData metadata;
    memset(&metadata, 0, sizeof (metadata));

    metadata.parent.type = kH264MimeType;
    metadata.parent.width = config.width;
    metadata.parent.height = config.height;
    metadata.parent.fps = config.framerate;
    metadata.parent.flags = (DroidMediaCodecFlags)(DROID_MEDIA_CODEC_HW_ONLY | DROID_MEDIA_CODEC_USE_EXTERNAL_LOOP);
    metadata.bitrate = kDefaultBitrate;
    metadata.stride = config.width;
    metadata.slice_height = config.height;
    metadata.color_format = 0x7F000789;
    metadata.max_input_size = maxInputSize;
    metadata.meta_data = kMetadataBufferTypeInvalid;

    codec_ = droid_media_codec_create_encoder(&metadata);
    if (!codec_) {
        AC_ERROR("Failed to configure encoder");
        return false;
    }

    DroidMediaCodecCallbacks cb;
    cb.signal_eos = signal_eos;
    cb.error = signal_error;
    droid_media_codec_set_callbacks(codec_, &cb, this);

    DroidMediaCodecDataCallbacks ccb;
    ccb.data_available = data_available;
    droid_media_codec_set_data_callbacks(codec_, &ccb, this);

    config_ = config;

    AC_DEBUG("Configured encoder succesfully");

    return true;
}

bool H264Encoder::Start() {
    if (!codec_ || running_)
        return false;

    running_ = true;

    if (!droid_media_codec_start(codec_)) {
        AC_ERROR("Failed to start codec");
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

    droid_media_codec_loop(codec_);

    return running_;
}

bool H264Encoder::Stop() {
    if (!codec_ || !running_)
        return false;

    running_ = false;

    if (codec_)
        droid_media_codec_stop(codec_);

    report_->Stopped();

    return true;
}

static void rgb2yuv420p(char* rgb, char* yuv420p, unsigned int width, unsigned int height)
{
  unsigned int i = 0;
  unsigned int numpixels = width * height;
  unsigned int ui = numpixels;
  unsigned int vi = numpixels + numpixels / 4;
  unsigned int s = 0;
  const unsigned int colors = 4;

#define sR (char)(rgb[s+2])
#define sG (char)(rgb[s+1])
#define sB (char)(rgb[s+0])

  for (int j = 0; j < height; j++)
    for (int k = 0; k < width; k++)
    {
      yuv420p[i] = (char)( (66*sR + 129*sG + 25*sB + 128) >> 8) + 16;

      if (0 == j%2 && 0 == k%2)
      {
        yuv420p[ui++] = (char)( (-38*sR - 74*sG + 112*sB + 128) >> 8) + 128;
        yuv420p[vi++] = (char)( (112*sR - 94*sG - 18*sB + 128) >> 8) + 128;
      }
      i++;
      s += colors;
    }
}

void H264Encoder::QueueBuffer(const video::Buffer::Ptr &input_buffer) {
    if (!running_)
        return;

    report_->ReceivedInputBuffer(input_buffer->Timestamp());
    AC_DEBUG("Queueing buffer");

    void* pixels = nullptr;
    uint32_t size = 0;
    DroidMediaBufferCallbacks cb;

    if (!input_buffer->NativeHandle() && input_buffer->Length() > 0) {
        size = (config_.width * config_.height * 3 / 2);
        pixels = new char[size];
        rgb2yuv420p((char*)input_buffer->Data(), (char*)pixels, config_.width, config_.height);

        cb.unref = signal_release_readout;
        cb.data = pixels;
    } else if (input_buffer->NativeHandle()) {
        // Import native handle into Droidmedia buffer
        AC_DEBUG("Importing buffer");
        const auto anwb = reinterpret_cast<ANativeWindowBuffer*>(input_buffer->NativeHandle());

        DroidMediaBuffer *imported_media_buffer =
            droid_media_buffer_import(anwb->handle, anwb->width, anwb->height, anwb->format,
                                      1, anwb->usage, anwb->stride);
        if (!imported_media_buffer) {
            AC_WARNING("Failed to import media buffer");
            return;
        }

        // Lock media buffer for readout
        uint32_t width = 0;
        uint32_t height = 0;
        //char* pixel_data = nullptr;

        pixels = droid_media_buffer_lock(imported_media_buffer, DROID_MEDIA_BUFFER_LOCK_READ);
        if (pixels) {
            width = droid_media_buffer_get_width(imported_media_buffer);
            height = droid_media_buffer_get_height(imported_media_buffer);
            size = width * height;
        } else {
            AC_WARNING("Buffer couldn't be locked");
            return;
        }

        cb.unref = signal_release;
        cb.data = imported_media_buffer;
    } else {
        AC_WARNING("Failed to queue input buffer");
        return;
    }

    // Create Droidmedia-compatible buffer object
    DroidMediaCodecData data;
    //data.data.size = size;
    //data.data.data = pixel_data;
    data.data.size = size;
    data.data.data = pixels;
    data.ts = input_buffer->Timestamp();
    data.sync = true;

    /*VideoNativeMetadata* native_md = new VideoNativeMetadata;
    native_md->eType = kMetadataBufferTypeANWBuffer;
    native_md->pBuffer = anwb;
    native_md->nFenceFd = -1;

    DroidMediaCodecData data;
    data.data.size = sizeof(VideoNativeMetadata);
    data.data.data = *native_md;
    data.ts = input_buffer->Timestamp();
    data.sync = false;*/

    AC_DEBUG("Enqueueing");
    droid_media_codec_queue(codec_, &data, &cb);
    AC_DEBUG("Enqueued");

    report_->BeganFrame(input_buffer->Timestamp());
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

} // namespace droidmedia
} // namespace ac
