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

#include <stdio.h>

#include "mcs/logger.h"

#include "mcs/video/statistics.h"

#include "mcs/streaming/mediasender.h"
#include "mcs/streaming/mpegtspacketizer.h"
#include "mcs/streaming/rtpsender.h"

namespace {
static constexpr const char *kMediaSenderThreadName{"MediaSender"};
}

namespace mcs {
namespace streaming {

MediaSender::Ptr MediaSender::Create(const Packetizer::Ptr &packetizer, const TransportSender::Ptr &sender, const mcs::video::BaseEncoder::Config &config) {
    return std::shared_ptr<MediaSender>(new MediaSender(packetizer, sender, config));
}

MediaSender::MediaSender(const Packetizer::Ptr &packetizer, const TransportSender::Ptr &sender, const mcs::video::BaseEncoder::Config &config) :
    packetizer_(packetizer),
    sender_(sender),
    prev_time_us_(-1ll),
    queue_(video::BufferQueue::Create()),
    running_(false) {

    if (!packetizer_ || !sender_) {
        MCS_WARNING("Sender not correct initialized. Missing packetizer or sender.");
        return;
    }

    // FIXME once we add support for audio this can be only used for video
    // and we need to differentiate here per track.
    Packetizer::TrackFormat format;
    format.profile_idc = config.profile_idc;
    format.level_idc = config.level_idc;
    format.constraint_set = config.constraint_set;
    format.mime = "video/avc";

    video_track_ = packetizer_->AddTrack(format);
}

MediaSender::~MediaSender() {
    Stop();
}

void MediaSender::Start() {
    if (running_)
        return;

    if (!sender_ || !packetizer_)
        return;

    running_ = true;
    worker_thread_ = std::thread(&MediaSender::WorkerThread, this);
    pthread_setname_np(worker_thread_.native_handle(), kMediaSenderThreadName);
}

void MediaSender::Stop() {
    if (!running_)
        return;

    running_ = false;
    worker_thread_.join();
}

void MediaSender::ProcessBuffer(const mcs::video::Buffer::Ptr &buffer) {
    mcs::video::Buffer::Ptr packets;

    static int64_t start_time_us = mcs::Utils::GetNowUs();
    static unsigned int buffer_count = 0;

    buffer_count++;
    int64_t time_now_us = mcs::Utils::GetNowUs();
    if (start_time_us + 1000000ll <= time_now_us) {
        video::Statistics::Instance()->RecordSenderBufferPerSecond(buffer_count);
        buffer_count = 0;
        start_time_us = time_now_us;
    }

    // FIXME: By default we're expecting the encoder to insert SPS and PPS
    // with each IDR frame but we need to handle also the case where the
    // encoder is not capable of doing this. For that we simply have to set
    // flags to Packetizer::kPrependSPSandPPStoIDRFrames.
    int flags = 0;

    // Per spec we need to emit PAT/PMT and PCR updates atleast every 100ms
    int64_t time_us = mcs::Utils::GetNowUs();
    if (prev_time_us_ < 0ll || prev_time_us_ + 100000ll <= time_us) {
        flags |= Packetizer::kEmitPATandPMT;
        flags |= Packetizer::kEmitPCR;
        prev_time_us_ = time_us;
    }

    if (!packetizer_->Packetize(video_track_, buffer, &packets, flags)) {
        MCS_ERROR("MPEGTS packetizing failed");
        return;
    }

    packets->SetTimestamp(buffer->Timestamp());
    sender_->Queue(packets);
}

void MediaSender::WorkerThread() {
    while (running_) {
        // This will wait for a short time and then return back
        // so we can loop again and check if we have to exit or
        // not.
        if (!queue_->WaitToBeFilled())
            continue;

        auto buffer = queue_->Pop();
        ProcessBuffer(buffer);
    }
}

void MediaSender::OnBufferAvailable(const video::Buffer::Ptr &buffer) {
    queue_->Push(buffer);
}

void MediaSender::OnBufferWithCodecConfig(const video::Buffer::Ptr &buffer) {
    if (!packetizer_)
        return;

    packetizer_->SubmitCSD(video_track_, buffer);
}

uint16_t MediaSender::LocalRTPPort() const {
    if (!sender_)
        return 0;

    return sender_->LocalPort();
}

} // namespace streaming
} // namespace mcs
