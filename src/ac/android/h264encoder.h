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

#ifndef AC_ANDORID_ENCODER_H_
#define AC_ANDORID_ENCODER_H_

#include <memory>
#include <thread>

#include <hybris/media/media_codec_source_layer.h>

#include "ac/non_copyable.h"
#include "ac/utils.h"

#include "ac/common/executable.h"

#include "ac/video/baseencoder.h"
#include "ac/video/encoderreport.h"
#include "ac/video/bufferqueue.h"

namespace ac {
namespace android {

class H264Encoder : public video::BaseEncoder {
public:
    typedef std::shared_ptr<H264Encoder> Ptr;

    static BaseEncoder::Ptr Create(const video::EncoderReport::Ptr &report);

    ~H264Encoder();

    BaseEncoder::Config DefaultConfiguration() override;

    bool Configure(const BaseEncoder::Config &config) override;

    void QueueBuffer(const ac::video::Buffer::Ptr &buffer) override;

    bool Running() const override { return running_; }
    BaseEncoder::Config Configuration() const override;

    void SendIDRFrame() override;

    // From ac::common::Executable
    bool Start() override;
    bool Stop() override;
    bool Execute() override;
    std::string Name() const override;

private:
    H264Encoder(const video::EncoderReport::Ptr &report);

    bool DoesBufferContainCodecConfig(MediaBufferWrapper *buffer);

    MediaBufferWrapper* PackBuffer(const ac::video::Buffer::Ptr &input_buffer, const ac::TimestampUs &timestamp);

private:
    static int OnSourceRead(MediaBufferWrapper **buffer, void *user_data);
    static int OnSourceStart(MediaMetaDataWrapper *meta, void *user_data);
    static int OnSourceStop(void *user_data);
    static int OnSourcePause(void *user_data);

    static void OnBufferReturned(MediaBufferWrapper *buffer, void *user_data);

private:
    struct BufferItem {
        ac::video::Buffer::Ptr buffer;
        MediaBufferWrapper *media_buffer;
    };

private:
    video::EncoderReport::Ptr report_;
    BaseEncoder::Config config_;
    MediaMessageWrapper *format_;
    MediaMetaDataWrapper *source_format_;
    MediaCodecSourceWrapper *encoder_;
    bool running_;
    ac::video::BufferQueue::Ptr input_queue_;
    std::vector<BufferItem> pending_buffers_;
    ac::TimestampUs start_time_;
    uint32_t frame_count_;
};

} // namespace android
} // namespace ac

#endif
