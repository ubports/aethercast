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

#include <mcs/video/h264analyzer.h>

TEST(H264Analyzer, DetectSPSCorrect) {
    mcs::video::H264Analyzer analyzer;

    uint8_t sps[] {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2};

    auto result = analyzer.Process(sps, 11);

    EXPECT_EQ(1, result.units);
    EXPECT_EQ(0, result.slices);
    EXPECT_EQ(0, result.idr_frames);
    EXPECT_EQ(1, result.sps);
    EXPECT_EQ(0, result.pps);
}


TEST(H264Analyzer, DetectPPSCorrect) {
    mcs::video::H264Analyzer analyzer;

    uint8_t pps[] { 0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80 };

    auto result = analyzer.Process(pps, 8);

    EXPECT_EQ(1, result.units);
    EXPECT_EQ(0, result.slices);
    EXPECT_EQ(0, result.idr_frames);
    EXPECT_EQ(0, result.sps);
    EXPECT_EQ(1, result.pps);
}

TEST(H264Analyzer, DetectMultipleUnits) {
    mcs::video::H264Analyzer analyzer;

    uint8_t multiple_units[] {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2,
        0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80
    };

    auto result = analyzer.Process(multiple_units, 19);

    EXPECT_EQ(2, result.units);
    EXPECT_EQ(0, result.slices);
    EXPECT_EQ(0, result.idr_frames);
    EXPECT_EQ(1, result.sps);
    EXPECT_EQ(1, result.pps);
}

TEST(H264Analyzer, CollectStatistics) {
    mcs::video::H264Analyzer analyzer(true);

    uint8_t multiple_units[] {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2,
        0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80
    };

    analyzer.Process(multiple_units, 19);
    analyzer.Process(multiple_units, 19);
    analyzer.Process(multiple_units, 19);

    auto statistics = analyzer.Statistics();

    EXPECT_EQ(6, statistics.units);
    EXPECT_EQ(0, statistics.slices);
    EXPECT_EQ(0, statistics.idr_frames);
    EXPECT_EQ(3, statistics.sps);
    EXPECT_EQ(3, statistics.pps);
}
