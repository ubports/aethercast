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

#ifndef MCS_STREAMING_MEDIASENDER_H_
#define MCS_STREAMING_MEDIASENDER_H_

#include <memory>
#include <mutex>
#include <thread>

#include "mcs/video/baseencoder.h"
#include "mcs/video/bufferqueue.h"

#include "mcs/streaming/packetizer.h"
#include "mcs/streaming/transportsender.h"

namespace mcs {
namespace streaming {

class MediaSender : public std::enable_shared_from_this<MediaSender>,
                    public mcs::video::BaseEncoder::Delegate {
public:
    typedef std::shared_ptr<MediaSender> Ptr;

    static Ptr Create(const Packetizer::Ptr &packetizer, const TransportSender::Ptr &sender, const mcs::video::BaseEncoder::Config &config);

    ~MediaSender();

    void Start();
    void Stop();

    void OnBufferAvailable(const mcs::video::Buffer::Ptr &buffer) override;
    void OnBufferWithCodecConfig(const mcs::video::Buffer::Ptr &buffer) override;

    uint16_t LocalRTPPort() const;

private:
    MediaSender(const Packetizer::Ptr &packetizer, const TransportSender::Ptr &sender, const mcs::video::BaseEncoder::Config &config);

    void WorkerThread();

    void ProcessBuffer(const mcs::video::Buffer::Ptr &buffer);

private:
    Packetizer::Ptr packetizer_;
    TransportSender::Ptr sender_;
    Packetizer::TrackId video_track_;
    int64_t prev_time_us_;
    std::thread worker_thread_;
    mcs::video::BufferQueue::Ptr queue_;
    bool running_;
};

} // namespace streaming
} // namespace mcs

#endif
