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

#include "mcs/logger.h"

#include "mcs/video/statistics.h"

namespace mcs {
namespace video {

Statistics::Ptr Statistics::Instance() {
    static Ptr instance = std::shared_ptr<Statistics>(new Statistics);
    return instance;
}

Statistics::Statistics() {
    renderer_wait_(0);
    renderer_swapped_(0);
    renderer_iteration_(0);
    encoder_buffer_out_(0);
    sender_buffer_per_second_(0);
    rtp_buffer_queued_(0);
    rtp_buffer_sent_(0);
    rtp_bandwidth_(0);
}

void Statistics::RecordRendererWait(int64_t timespan) {
    renderer_wait_(timespan);
}

void Statistics::RecordRendererSwapped(int64_t timespan) {
    renderer_swapped_(timespan);
}

void Statistics::RecordRendererIteration(int64_t timespan) {
    renderer_iteration_(timespan);
}

void Statistics::RecordRendererFramesPerSecond(int count) {
    renderer_frames_per_second_(count);
}

void Statistics::RecordEncoderBufferOut(int64_t timespan) {
    encoder_buffer_out_(timespan);
}

void Statistics::RecordSenderBufferPerSecond(int count) {
    sender_buffer_per_second_(count);
}

void Statistics::RecordRTPBufferQueued(int64_t timespan) {
    rtp_buffer_queued_(timespan);
}

void Statistics::RecordRTPBufferSent(int64_t timespan) {
    rtp_buffer_sent_(timespan);
}

void Statistics::RecordRTPBandwidth(int64_t mbit) {
    rtp_bandwidth_(mbit);
}

void Statistics::Dump() {
    MCS_DEBUG("Renderer wait: min = %lld ms max = %lld ms mean = %lld ms variance = %lld ms",
              ba::min(renderer_wait_),
              ba::max(renderer_wait_),
              ba::mean(renderer_wait_),
              ba::variance(renderer_wait_));

    MCS_DEBUG("Renderer swapped: min = %lld ms max = %lld ms mean = %lld ms variance = %lld ms",
              ba::min(renderer_swapped_),
              ba::max(renderer_swapped_),
              ba::mean(renderer_swapped_),
              ba::variance(renderer_swapped_));

    MCS_DEBUG("Renderer frames per second: min = %lld max = %lld mean = %lld variance = %lld",
              ba::min(renderer_frames_per_second_),
              ba::max(renderer_frames_per_second_),
              ba::mean(renderer_frames_per_second_),
              ba::variance(renderer_frames_per_second_));

    MCS_DEBUG("Renderer iteration: min = %lld ms max = %lld ms mean = %lld ms variance = %lld ms",
              ba::min(renderer_iteration_),
              ba::max(renderer_iteration_),
              ba::mean(renderer_iteration_),
              ba::variance(renderer_iteration_));

    MCS_DEBUG("Encoder buffer out: min = %lld ms max = %lld ms mean = %lld ms variance = %lld ms",
              ba::min(encoder_buffer_out_),
              ba::max(encoder_buffer_out_),
              ba::mean(encoder_buffer_out_),
              ba::variance(encoder_buffer_out_));

    MCS_DEBUG("Sender buffer per second: min = %lld max = %lld mean = %lld variance = %lld",
              ba::min(sender_buffer_per_second_),
              ba::max(sender_buffer_per_second_),
              ba::mean(sender_buffer_per_second_),
              ba::variance(sender_buffer_per_second_));

    MCS_DEBUG("RTP buffer queued: min = %lld ms max = %lld ms mean = %lld ms variance = %lld ms",
              ba::min(rtp_buffer_queued_),
              ba::max(rtp_buffer_queued_),
              ba::mean(rtp_buffer_queued_),
              ba::variance(rtp_buffer_queued_));

    MCS_DEBUG("RTP buffer sent: min = %lld ms max = %lld ms mean = %lld ms variance = %lld ms",
              ba::min(rtp_buffer_sent_),
              ba::max(rtp_buffer_sent_),
              ba::mean(rtp_buffer_sent_),
              ba::variance(rtp_buffer_sent_));

    MCS_DEBUG("RTP send bandwidth: min = %lld MBit/s max = %lld MBit/s mean = %lld MBit/s variance = %lld MBit/s",
              ba::min(rtp_bandwidth_),
              ba::max(rtp_bandwidth_),
              ba::mean(rtp_bandwidth_),
              ba::variance(rtp_bandwidth_));
}

} // namespace video
} // namespace mcs
