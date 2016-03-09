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

#include <algorithm>

#include <gtest/gtest.h>

#include <wds/video_format.h>

std::vector<wds::H264VideoCodec> GetLocalH264VideoCodecs() {
    static std::vector<wds::H264VideoCodec> codecs;
    if (codecs.empty()) {
        wds::RateAndResolutionsBitmap cea_rr;
        wds::RateAndResolutionsBitmap vesa_rr;
        wds::RateAndResolutionsBitmap hh_rr;
        wds::RateAndResolution i;

        for (i = wds::CEA640x480p60; i <= wds::CEA1920x1080p24; ++i)
            cea_rr.set(i);
        for (i = wds::VESA800x600p30; i <= wds::VESA1920x1200p30; ++i)
            vesa_rr.set(i);
        for (i = wds::HH800x480p30; i <= wds::HH848x480p60; ++i)
            hh_rr.set(i);

        wds::H264VideoCodec codec(wds::CHP, wds::k4_2, cea_rr, vesa_rr, hh_rr);
        codecs.push_back(codec);
    }

    return codecs;
}

TEST(VideoFormatSelection, SelectsExpectedFormat) {
    wds::RateAndResolutionsBitmap cea_rr;
    wds::RateAndResolutionsBitmap vesa_rr;
    wds::RateAndResolutionsBitmap hh_rr;
    wds::RateAndResolution i;

    for (i = wds::CEA640x480p60; i <= wds::CEA1920x1080p24; ++i)
      cea_rr.set(i);

    std::vector<wds::H264VideoCodec> sink_codecs;
    wds::H264VideoCodec sink_codec(wds::CBP, wds::k3_2, cea_rr, vesa_rr, hh_rr);
    sink_codecs.push_back(sink_codec);

    wds::NativeVideoFormat sink_native_format;
    sink_native_format.type = wds::CEA;
    sink_native_format.rate_resolution = wds::CEA1920x1080p60;

    bool success = false;

    wds::H264VideoFormat format = wds::FindOptimalVideoFormat(sink_native_format, GetLocalH264VideoCodecs(), sink_codecs, &success);

    EXPECT_TRUE(success);
    EXPECT_EQ(wds::k3_2, format.level);
    EXPECT_EQ(wds::CBP, format.profile);
    EXPECT_EQ(wds::CEA, format.type);
    EXPECT_EQ(wds::CEA1920x1080p60, format.rate_resolution);
}
