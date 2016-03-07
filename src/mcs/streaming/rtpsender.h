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

#ifndef MCS_STREAMING_RTPSENDER_H_
#define MCS_STREAMING_RTPSENDER_H_

#include <gio/gio.h>

#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "mcs/scoped_gobject.h"

#include "mcs/video/buffer.h"
#include "mcs/video/bufferqueue.h"
#include "mcs/video/senderreport.h"

#include "mcs/streaming/transportsender.h"

namespace mcs {
namespace streaming {

class RTPSender : public std::enable_shared_from_this<RTPSender>,
                  public TransportSender {
public:
    static TransportSender::Ptr Create(const std::string &address, int port,
                                       const video::SenderReport::Ptr &report);

    ~RTPSender();

    bool Queue(const mcs::video::Buffer::Ptr &packets) override;

    int32_t LocalPort() const override;

private:
    RTPSender(const video::SenderReport::Ptr &report);

    RTPSender::Ptr FinalizerConstruction(const std::string &address, int port);

    void ThreadLoop();

private:
    video::SenderReport::Ptr report_;
    bool running_;
    int fd_;
    uint32_t rtp_sequence_number_;
    std::thread worker_thread_;
    mcs::video::BufferQueue::Ptr queue_;
    int32_t local_port_;
};

} // namespace streaming
} // namespace mcs

#endif
