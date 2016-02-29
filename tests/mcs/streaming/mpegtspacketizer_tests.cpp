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

#include <mcs/streaming/mpegtspacketizer.h>

namespace {
static constexpr uint8_t kMPEGTSPacketLength = 188;
static constexpr uint8_t kMPEGTSStartByte = 0x47;
static const uint8_t csd0[] = {
    // SPS
    0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2,
    // PPS
    0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80
};
static const uint8_t frame0[] = {
    // Slice header comes first
    0x00, 0x00, 0x00, 0x01, 0x05, 0x88, 0x84, 0x21, 0xa0,
    // Macro block header
    0x0d, 0x00,
    // Macro block
    0x01, 0x2, 0x3, 0x4, 0x5, 0x6
};
}

TEST(MPEGTSPacketizer, AddTrackWithoutAnythingSet) {
    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create();
    auto id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{});
    EXPECT_EQ(-1, id);
}

TEST(MPEGTSPacketizer, AddValidTrack) {
    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create();
    auto id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{"video/avc"});
    EXPECT_EQ(0, id);
}

TEST(MPEGTSPacketizer, TryMoreThanOneValidTrack) {
    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create();
    auto id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{"video/avc"});
    EXPECT_EQ(0, id);
    for (int i = 0; i < 15; i++) {
        id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{"video/avc"});
        EXPECT_EQ(i+1, id);
    }
    id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{"video/avc"});
    EXPECT_EQ(-1, id);
}

TEST(MPEGTSPacketizer, SubmitAndProcessCodecSpecificData) {
    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create();
    auto id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{"video/avc"});

    auto buffer = mcs::video::Buffer::Create(sizeof(csd0));
    ::memcpy(buffer->Data(), csd0, sizeof(csd0));

    mcs::video::Buffer::Ptr out;

    packetizer->Packetize(id, buffer, &out);

    EXPECT_NE(nullptr, out.get());

    // One MPEGTS element has a constant length
    EXPECT_EQ(kMPEGTSPacketLength, out->Length());
    // .. and starts with a special byte
    EXPECT_EQ(kMPEGTSStartByte, out->Data()[0]);

    // They encode the header with the
    uint8_t expected_bytes[] = { 0x47, 0x50, 0x11, 0x30, 0x96, 0x00 };

    // What we have supplied into the packtizer should be now placed at the end of
    // the element we got back from the packetizer.
    EXPECT_EQ(0, memcmp(out->Data() + (out->Length() - buffer->Length()),
                        buffer->Data(), buffer->Length()));

    std::cout << "length: " << out->Length() << std::endl;
    std::cout << mcs::Utils::Hexdump(out->Data(), out->Length()) << std::endl;
}
