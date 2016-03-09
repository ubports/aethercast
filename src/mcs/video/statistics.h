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

#ifndef MCS_VIDEO_STATISTICS_H_
#define MCS_VIDEO_STATISTICS_H_

#include <stdint.h>

#include <memory>
#include <chrono>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>

namespace ba = boost::accumulators;

namespace mcs {
namespace video {

class Statistics {
public:
    typedef std::shared_ptr<Statistics> Ptr;

    static Ptr Instance();

    void RecordRendererIteration(int64_t timespan);
    void RecordRendererWait(int64_t timespan);
    void RecordRendererSwapped(int64_t timespan);
    void RecordRendererFramesPerSecond(int count);
    void RecordEncoderBufferOut(int64_t timespan);
    void RecordRTPBufferQueued(int64_t timespan);
    void RecordRTPBufferSent(int64_t timespan);
    void RecordRTPBandwidth(int64_t mbit);
    void RecordSenderBufferPerSecond(int count);

    void Dump();

private:
    Statistics();

private:
    ba::accumulator_set<int64_t, ba::features<ba::tag::min, ba::tag::mean, ba::tag::max, ba::tag::variance>> renderer_iteration_;
    ba::accumulator_set<int64_t, ba::features<ba::tag::min, ba::tag::mean, ba::tag::max, ba::tag::variance>> renderer_wait_;
    ba::accumulator_set<int64_t, ba::features<ba::tag::min, ba::tag::mean, ba::tag::max, ba::tag::variance>> renderer_swapped_;
    ba::accumulator_set<int64_t, ba::features<ba::tag::min, ba::tag::mean, ba::tag::max, ba::tag::variance>> renderer_frames_per_second_;
    ba::accumulator_set<int64_t, ba::features<ba::tag::min, ba::tag::mean, ba::tag::max, ba::tag::variance>> encoder_buffer_out_;
    ba::accumulator_set<int64_t, ba::features<ba::tag::min, ba::tag::mean, ba::tag::max, ba::tag::variance>> sender_buffer_per_second_;
    ba::accumulator_set<int64_t, ba::features<ba::tag::min, ba::tag::mean, ba::tag::max, ba::tag::variance>> rtp_buffer_queued_;
    ba::accumulator_set<int64_t, ba::features<ba::tag::min, ba::tag::mean, ba::tag::max, ba::tag::variance>> rtp_buffer_sent_;
    ba::accumulator_set<int64_t, ba::features<ba::tag::min, ba::tag::mean, ba::tag::max, ba::tag::variance>> rtp_bandwidth_;
};

} // namespace video
} // namespace mcs

#endif
