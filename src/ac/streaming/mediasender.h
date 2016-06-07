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

#ifndef AC_STREAMING_MEDIASENDER_H_
#define AC_STREAMING_MEDIASENDER_H_

#include <memory>
#include <mutex>
#include <thread>

#include "ac/video/baseencoder.h"
#include "ac/video/bufferqueue.h"

#include "ac/streaming/packetizer.h"
#include "ac/streaming/transportsender.h"

namespace ac {
namespace streaming {

class MediaSender : public ac::common::Executable,
                    public ac::video::BaseEncoder::Delegate {
public:
    typedef std::shared_ptr<MediaSender> Ptr;

    MediaSender(const Packetizer::Ptr &packetizer, const TransportSender::Ptr &sender,
                const ac::video::BaseEncoder::Config &config);
    ~MediaSender();

    uint16_t LocalRTPPort() const;

    // From ac::common::Executable
    bool Start() override;
    bool Stop() override;
    bool Execute() override;
    std::string Name() const override;

    // From ac::video::BaseEncoder::Delegate
    void OnBufferAvailable(const ac::video::Buffer::Ptr &buffer) override;
    void OnBufferWithCodecConfig(const ac::video::Buffer::Ptr &buffer) override;

private:
    void WorkerThread();

    void ProcessBuffer(const ac::video::Buffer::Ptr &buffer);

private:
    Packetizer::Ptr packetizer_;
    TransportSender::Ptr sender_;
    Packetizer::TrackId video_track_;
    int64_t prev_time_us_;
    ac::video::BufferQueue::Ptr queue_;
};

} // namespace streaming
} // namespace ac

#endif
