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

#ifndef MCS_ANDORID_ENCODER_H_
#define MCS_ANDORID_ENCODER_H_

#include <memory>
#include <thread>

#include <hybris/media/media_codec_source_layer.h>

#include "mcs/non_copyable.h"
#include "mcs/utils.h"
#include "mcs/common/threadpolicy.h"

#include "mcs/video/baseencoder.h"
#include "mcs/video/bufferqueue.h"

#include "mcs/android/mediaapi.h"

namespace mcs {
namespace android {

class H264Encoder : public video::BaseEncoder {
public:
    typedef std::shared_ptr<H264Encoder> Ptr;

    static BaseEncoder::Config DefaultConfiguration();

    static BaseEncoder::Ptr Create(const MediaAPI::Ptr &api = MediaAPI::CreateDefault());

    ~H264Encoder();

    bool IsValid() const override;

    bool Configure(const BaseEncoder::Config &config);

    bool Start() override;
    bool Stop() override;

    void QueueBuffer(const mcs::video::Buffer::Ptr &buffer) override;

    bool Running() const override { return running_; }
    void* NativeWindowHandle() const override;
    BaseEncoder::Config Configuration() const override;

    void SendIDRFrame() override;

private:
    H264Encoder(const MediaAPI::Ptr &api);

    void WorkerThread();

    bool DoesBufferContainCodecConfig(MediaBufferWrapper *buffer);

    MediaBufferWrapper* PackBuffer(const mcs::video::Buffer::Ptr &input_buffer, const mcs::TimestampUs &timestamp);

private:
    static int OnSourceStart(MediaMetaDataWrapper *meta, void *user_data);
    static int OnSourceStop(void *user_data);
    static int OnSourceRead(MediaBufferWrapper **buffer, void *user_data);
    static int OnSourcePause(void *user_data);

    static void OnBufferReturned(MediaBufferWrapper *buffer, void *user_data);

private:
    struct BufferItem {
        mcs::video::Buffer::Ptr buffer;
        MediaBufferWrapper *media_buffer;
    };

private:
    MediaAPI::Ptr api_;
    BaseEncoder::Config config_;
    MediaMessageWrapper *format_;
    MediaSourceWrapper *source_;
    MediaCodecSourceWrapper *encoder_;
    MediaMetaDataWrapper *source_format_;
    bool running_;
    std::thread worker_thread_;
    mcs::video::BufferQueue::Ptr input_queue_;
    std::vector<BufferItem> pending_buffers_;
    mcs::TimestampUs start_time_;
    uint32_t frame_count_;
};

} // namespace android
} // namespace mcs

#endif
