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

#include "ac/logger.h"

#include "ac/streaming/mediasender.h"
#include "ac/streaming/mpegtspacketizer.h"
#include "ac/streaming/rtpsender.h"

namespace {
static constexpr const char *kMediaSenderThreadName{"MediaSender"};
}

namespace ac {
namespace streaming {

MediaSender::MediaSender(const Packetizer::Ptr &packetizer, const TransportSender::Ptr &sender,
                         const ac::video::BaseEncoder::Config &config) :
    packetizer_(packetizer),
    sender_(sender),
    prev_time_us_(-1ll),
    queue_(video::BufferQueue::Create()) {

    if (!packetizer_ || !sender_) {
        AC_WARNING("Sender not correct initialized. Missing packetizer or sender.");
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

bool MediaSender::Start() {
    return true;
}

bool MediaSender::Stop() {
    return true;
}

void MediaSender::ProcessBuffer(const ac::video::Buffer::Ptr &buffer) {
    ac::video::Buffer::Ptr packets;

    // FIXME: By default we're expecting the encoder to insert SPS and PPS
    // with each IDR frame but we need to handle also the case where the
    // encoder is not capable of doing this. For that we simply have to set
    // flags to Packetizer::kPrependSPSandPPStoIDRFrames.
    int flags = 0;

    // Per spec we need to emit PAT/PMT and PCR updates atleast every 100ms
    int64_t time_us = ac::Utils::GetNowUs();
    if (prev_time_us_ < 0ll || prev_time_us_ + 100000ll <= time_us) {
        flags |= Packetizer::kEmitPATandPMT;
        flags |= Packetizer::kEmitPCR;
        prev_time_us_ = time_us;
    }

    if (!packetizer_->Packetize(video_track_, buffer, &packets, flags)) {
        AC_ERROR("MPEGTS packetizing failed");
        return;
    }

    packets->SetTimestamp(buffer->Timestamp());
    sender_->Queue(packets);
}

bool MediaSender::Execute() {
    // This will wait for a short time and then return back
    // so we can loop again and check if we have to exit or
    // not.
    if (!queue_->WaitToBeFilled())
        return true;

    const auto buffer = queue_->Pop();
    ProcessBuffer(buffer);

    return true;
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

std::string MediaSender::Name() const {
    return kMediaSenderThreadName;
}

} // namespace streaming
} // namespace ac
