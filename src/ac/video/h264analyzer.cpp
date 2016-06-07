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

#include <fstream>
#include <sstream>

#include "ac/video/utils.h"
#include "ac/video/h264analyzer.h"

namespace {
enum {
    kNalTypeSlice = 1,
    kNalTypeIDR = 5,
    kNalTypeSPS = 7,
    kNalTypePPS = 8,
};
}

namespace ac {
namespace video {

H264Analyzer::Result::Result() :
    units(0),
    slices(0),
    idr_frames(0),
    sps(0),
    pps(0) {
}

H264Analyzer::Result::Result(const H264Analyzer::Result &other) :
    units(other.units),
    slices(other.slices),
    idr_frames(other.idr_frames),
    sps(other.sps),
    pps(other.pps) {
}

H264Analyzer::Result& H264Analyzer::Result::operator+=(const H264Analyzer::Result& rhs) {
    units += rhs.units;
    slices += rhs.slices;
    idr_frames += rhs.idr_frames;
    sps += rhs.sps;
    pps += rhs.pps;
    return *this;
}

std::ostream& operator<<(std::ostream& out, const H264Analyzer::Result &rhs) {
    return out << "units " << rhs.units
               << " slices " << rhs.slices
               << " idr frames " << rhs.idr_frames
               << " sps " << rhs.sps
               << " pps " << rhs.pps;
}

H264Analyzer::H264Analyzer(bool collect_statistics) :
    collect_statistics_(collect_statistics) {
}

H264Analyzer::~H264Analyzer() {
}

H264Analyzer::Result H264Analyzer::Process(const uint8_t *data, size_t size) {
    const uint8_t *nal_start = nullptr;
    size_t nal_size = 0;
    Result result;

    while (GetNextNALUnit(&data, &size, &nal_start, &nal_size, true)) {
        unsigned nal_type = nal_start[0] & 0x1f;

        result.units++;

        switch (nal_type) {
        case kNalTypeSlice:
            result.slices++;
            break;
        case kNalTypeIDR:
            result.idr_frames++;
            break;
        case kNalTypeSPS:
            result.sps++;
            break;
        case kNalTypePPS:
            result.pps++;
            break;
        default:
            break;
        }
    }

    if (collect_statistics_)
        statistics_ += result;

    return result;
}

H264Analyzer::Result H264Analyzer::Statistics() const {
    return statistics_;
}

} // namespace video
} // namespace ac
