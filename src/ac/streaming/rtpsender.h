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

#ifndef AC_STREAMING_RTPSENDER_H_
#define AC_STREAMING_RTPSENDER_H_

#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "ac/common/executable.h"

#include "ac/network/stream.h"

#include "ac/video/buffer.h"
#include "ac/video/bufferqueue.h"
#include "ac/video/senderreport.h"

#include "ac/streaming/transportsender.h"

namespace ac {
namespace streaming {

class RTPSender : public TransportSender,
                  public common::Executable {
public:
    RTPSender(const network::Stream::Ptr &stream, const video::SenderReport::Ptr &report);
    ~RTPSender();

    // From ac::streaming::TransportSender
    bool Queue(const ac::video::Buffer::Ptr &packets) override;
    int32_t LocalPort() const override;

    // From ac::common::Executable
    bool Start() override;
    bool Stop() override;
    bool Execute() override;
    std::string Name() const override;

private:
    network::Stream::Ptr stream_;
    const std::uint32_t max_ts_packets_;
    video::SenderReport::Ptr report_;
    uint16_t rtp_sequence_number_;
    ac::video::BufferQueue::Ptr queue_;
    std::atomic<bool> network_error_;
};

} // namespace streaming
} // namespace ac

#endif
