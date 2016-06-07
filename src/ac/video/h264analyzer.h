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

#ifndef AC_VIDEO_H264ANALYZER_H_
#define AC_VIDEO_H264ANALYZER_H_

#include <memory>
#include <stdint.h>

namespace ac {
namespace video {

class H264Analyzer {
public:
    class Result {
    public:
        Result();
        Result(const Result &other);

        unsigned int units;
        unsigned int slices;
        unsigned int idr_frames;
        unsigned int sps;
        unsigned int pps;

        Result& operator+=(const Result& rhs);
    };

    H264Analyzer(bool collect_stats = false);
    ~H264Analyzer();

    Result Process(const uint8_t *data, size_t size);

    Result Statistics() const;

private:
    bool collect_statistics_;
    Result statistics_;
};

std::ostream& operator<<(std::ostream& out, const H264Analyzer::Result &rhs);

} // namespace video
} // namespace ac

#endif
