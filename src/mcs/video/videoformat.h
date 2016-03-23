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

#ifndef MCS_VIDEOFORMAT_H_
#define MCS_VIDEOFORMAT_H_

#include <wds/video_format.h>

namespace mcs {
namespace video {

void DumpVideoCodec(const wds::H264VideoCodec &codec);
void DumpVideoFormat(const wds::H264VideoFormat &format);
void DumpNativeFormat(const wds::NativeVideoFormat &format);

struct RateAndResolution {
    unsigned int width;
    unsigned int height;
    double framerate;
};

RateAndResolution ExtractRateAndResolution(const wds::H264VideoFormat &format);
std::string ExtractH264Profile(const wds::H264VideoFormat &format);
std::string ExtractH264Level(const wds::H264VideoFormat &format);

void ExtractProfileLevel(const wds::H264VideoFormat &format, int *profile,
                         int *level, int *constraint);

} // namespace video
} // namespace mcs

#endif
