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

#include <gtest/gtest.h>

#include "ac/video/videoformat.h"

namespace {
template<typename T>
void ExtractAndVerifyRateResolution(T rr, int width, int height, int framerate) {
    auto format = wds::H264VideoFormat{wds::CBP, wds::k3_1, rr};
    auto result = ac::video::ExtractRateAndResolution(format);
    EXPECT_EQ(width, result.width);
    EXPECT_EQ(height, result.height);
    EXPECT_EQ(framerate, result.framerate);
}
}

TEST(VideoFormat, ExtractRateAndResolution) {
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA640x480p60, 640, 480, 60);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA720x480p60, 720, 480, 60);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA720x480i60, 720, 480, 60);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA720x576p50, 720, 576, 50);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA720x576i50, 720, 576, 50);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1280x720p30, 1280, 720, 30);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1280x720p60, 1280, 720, 60);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1920x1080p30, 1920, 1080, 30);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1920x1080p60, 1920, 1080, 60);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1920x1080i60, 1920, 1080, 60);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1280x720p25, 1280, 720, 25);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1280x720p50, 1280, 720, 50);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1920x1080p25, 1920, 1080, 25);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1920x1080p50, 1920, 1080, 50);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1920x1080i50, 1920, 1080, 50);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1280x720p24, 1280, 720, 24);
    ExtractAndVerifyRateResolution<wds::CEARatesAndResolutions>(wds::CEA1920x1080p24, 1920, 1080, 24);

#if 0
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA800x600p30, 800, 600, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA800x600p60, 800, 600, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1024x768p30, 1024, 768, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1024x768p60, 1024, 768, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1152x864p30, 1152, 864, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1152x864p60, 1152, 864, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1280x768p30, 1280, 768, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1280x768p60, 1280, 768, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1280x800p30, 1280, 800, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1280x800p60, 1280, 800, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1360x768p30, 1360, 768, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1360x768p60, 1360, 768, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1366x768p30, 1366, 768, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1366x768p60, 1366, 768, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1280x1024p30, 1280, 1024, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1280x1024p60, 1280, 1024, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1400x1050p30, 1400, 1050, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1400x1050p60, 1400, 1050, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1440x900p30, 1440, 900, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1440x900p60, 1440, 900, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1600x900p30, 1600, 900, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1600x900p60, 1600, 900, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1600x1200p30, 1600, 1200, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1600x1200p60, 1600, 1200, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1680x1024p30, 1680, 1024, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1680x1024p60, 1680, 1024, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1680x1050p30, 1680, 1050, 30);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1680x1050p60, 1680, 1050, 60);
    ExtractAndVerifyRateResolution<wds::VESARatesAndResolutions>(wds::VESA1920x1200p30, 1920, 1200, 30);

    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH800x480p30, 800, 480, 30);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH800x480p60, 800, 480, 60);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH854x480p30, 854, 480, 30);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH854x480p60, 854, 480, 60);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH864x480p30, 864, 480, 30);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH864x480p60, 864, 480, 60);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH640x360p30, 640, 360, 30);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH640x360p60, 640, 360, 60);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH960x540p30, 960, 540, 30);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH960x540p60, 960, 540, 60);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH848x480p30, 848, 480, 30);
    ExtractAndVerifyRateResolution<wds::HHRatesAndResolutions>(wds::HH848x480p60, 848, 480, 60);
#endif
}

TEST(VideoFormat, ExtractH264Profile) {
    EXPECT_EQ("constrained-baseline", ac::video::ExtractH264Profile(wds::H264VideoFormat{wds::CBP, wds::k3_1, wds::CEA640x480p60}));
    EXPECT_EQ("high", ac::video::ExtractH264Profile(wds::H264VideoFormat{wds::CHP, wds::k3_1, wds::CEA640x480p60}));
}

TEST(VideoFormat, ExtractH264Level) {
    EXPECT_EQ("3.1", ac::video::ExtractH264Level(wds::H264VideoFormat{wds::CBP, wds::k3_1, wds::CEA640x480p60}));
    EXPECT_EQ("3.2", ac::video::ExtractH264Level(wds::H264VideoFormat{wds::CBP, wds::k3_2, wds::CEA640x480p60}));
    EXPECT_EQ("4", ac::video::ExtractH264Level(wds::H264VideoFormat{wds::CBP, wds::k4, wds::CEA640x480p60}));
    EXPECT_EQ("4.1", ac::video::ExtractH264Level(wds::H264VideoFormat{wds::CBP, wds::k4_1, wds::CEA640x480p60}));
    EXPECT_EQ("4.2", ac::video::ExtractH264Level(wds::H264VideoFormat{wds::CBP, wds::k4_2, wds::CEA640x480p60}));
}

TEST(VideoFormat, ExtractProfileLevel) {
    int profile = 0, level = 0, constraint = 0;

    ac::video::ExtractProfileLevel(wds::H264VideoFormat{wds::CBP, wds::k3_1, wds::CEA640x480p60},
                                    &profile, &level, &constraint);
    EXPECT_EQ(66, profile);
    EXPECT_EQ(0xc0, constraint);
    EXPECT_EQ(31, level);

    ac::video::ExtractProfileLevel(wds::H264VideoFormat{wds::CBP, wds::k3_2, wds::CEA640x480p60},
                                    &profile, &level, &constraint);
    EXPECT_EQ(66, profile);
    EXPECT_EQ(0xc0, constraint);
    EXPECT_EQ(32, level);

    ac::video::ExtractProfileLevel(wds::H264VideoFormat{wds::CBP, wds::k4, wds::CEA640x480p60},
                                    &profile, &level, &constraint);
    EXPECT_EQ(66, profile);
    EXPECT_EQ(0xc0, constraint);
    EXPECT_EQ(40, level);

    ac::video::ExtractProfileLevel(wds::H264VideoFormat{wds::CBP, wds::k4_1, wds::CEA640x480p60},
                                    &profile, &level, &constraint);
    EXPECT_EQ(66, profile);
    EXPECT_EQ(0xc0, constraint);
    EXPECT_EQ(41, level);

    ac::video::ExtractProfileLevel(wds::H264VideoFormat{wds::CBP, wds::k4_2, wds::CEA640x480p60},
                                    &profile, &level, &constraint);
    EXPECT_EQ(66, profile);
    EXPECT_EQ(0xc0, constraint);
    EXPECT_EQ(42, level);

    ac::video::ExtractProfileLevel(wds::H264VideoFormat{wds::CHP, wds::k3_1, wds::CEA640x480p60},
                                    &profile, &level, &constraint);
    EXPECT_EQ(100, profile);
    EXPECT_EQ(0x0c, constraint);
    EXPECT_EQ(31, level);

    ac::video::ExtractProfileLevel(wds::H264VideoFormat{wds::CHP, wds::k3_2, wds::CEA640x480p60},
                                    &profile, &level, &constraint);
    EXPECT_EQ(100, profile);
    EXPECT_EQ(0x0c, constraint);
    EXPECT_EQ(32, level);

    ac::video::ExtractProfileLevel(wds::H264VideoFormat{wds::CHP, wds::k4, wds::CEA640x480p60},
                                    &profile, &level, &constraint);
    EXPECT_EQ(100, profile);
    EXPECT_EQ(0x0c, constraint);
    EXPECT_EQ(40, level);

    ac::video::ExtractProfileLevel(wds::H264VideoFormat{wds::CHP, wds::k4_1, wds::CEA640x480p60},
                                    &profile, &level, &constraint);
    EXPECT_EQ(100, profile);
    EXPECT_EQ(0x0c, constraint);
    EXPECT_EQ(41, level);

    ac::video::ExtractProfileLevel(wds::H264VideoFormat{wds::CHP, wds::k4_2, wds::CEA640x480p60},
                                    &profile, &level, &constraint);
    EXPECT_EQ(100, profile);
    EXPECT_EQ(0x0c, constraint);
    EXPECT_EQ(42, level);
}
