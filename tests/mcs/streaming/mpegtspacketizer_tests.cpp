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

#include <gmock/gmock.h>

#include <chrono>

#include <mcs/streaming/mpegtspacketizer.h>

using namespace ::testing;

namespace {
static constexpr uint8_t kMPEGTSPacketLength = 188;
static constexpr uint8_t kMPEGTSStartByte = 0x47;
static const uint8_t csd0[] = {
    // SPS
    0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2,
    // PPS
    0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80
};
static const uint8_t slice_header[] = {
    // Slice header comes first
    0x00, 0x00, 0x00, 0x01, 0x05, 0x88, 0x84, 0x21, 0xa0,
};

mcs::video::Buffer::Ptr CreateFrame(int size) {
    auto buffer = mcs::video::Buffer::Create(size + sizeof(slice_header));
    ::memcpy(buffer->Data(), slice_header, sizeof(slice_header));
    return buffer;
}

class MPEGTSPacketMatcher {
public:
    MPEGTSPacketMatcher(const mcs::video::Buffer::Ptr &buffer) :
        buffer_(buffer) {
    }

    MPEGTSPacketMatcher(const MPEGTSPacketMatcher &other) :
        buffer_(other.buffer_) {
    }

    void ExpectValid() {
        EXPECT_EQ(0x47, buffer_->Data()[0]);
    }

    void ExpectPackets(int count) {
        // One MPEGTS element has a constant length
        EXPECT_EQ(kMPEGTSPacketLength * count, buffer_->Length());

        // Split into multiple packets so that those can be process
        // individually.
        for (int n = 0; n < count; n++) {
            auto packet = mcs::video::Buffer::Create(kMPEGTSPacketLength);
            ::memcpy(packet->Data(), buffer_->Data() + (n * kMPEGTSPacketLength), kMPEGTSPacketLength);
            packets_.push_back(MPEGTSPacketMatcher(packet));
        }
    }

    void ExpectPID(uint16_t expected_pid) {
        uint16_t pid = ((buffer_->Data()[1] ^ 0x40) << 8) | buffer_->Data()[2];
        EXPECT_EQ(expected_pid, pid);
    }

    void ExpectPaddingBytesAndContinuityCounter(uint8_t counter) {
        EXPECT_EQ(0x30 | counter, buffer_->Data()[3]);
    }

    void ExpectNoPaddingBytesAndContinuityCounter(uint8_t counter) {
        EXPECT_EQ(0x10 | counter, buffer_->Data()[3]);
    }

    void ExpectData(uint8_t *data, size_t length) {
        // What we have supplied into the packtizer should be now placed at the end of
        // the element we got back from the packetizer.
        EXPECT_EQ(0, memcmp(buffer_->Data() + (buffer_->Length() - length),
                            data, length));
    }

    void ExpectByte(unsigned int n, uint8_t expected_byte) {
        EXPECT_EQ(expected_byte, buffer_->Data()[n]);
    }

    MPEGTSPacketMatcher& At(int n) {
        return packets_.at(n);
    }

private:
    mcs::video::Buffer::Ptr buffer_;
    std::vector<MPEGTSPacketMatcher> packets_;
};

class MockPacketizerReport : public mcs::video::PacketizerReport {
public:
    MOCK_METHOD1(PacketizedFrame, void(const mcs::TimestampUs&));
};

}

TEST(MPEGTSPacketizer, AddTrackWithoutAnythingSet) {
    auto report = std::make_shared<MockPacketizerReport>();
    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create(report);
    auto id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{});
    EXPECT_EQ(-1, id);
}

TEST(MPEGTSPacketizer, AddValidTrack) {
    auto report = std::make_shared<MockPacketizerReport>();
    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create(report);
    auto id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{"video/avc"});
    EXPECT_EQ(0, id);
}

TEST(MPEGTSPacketizer, TryMoreThanOneValidTrack) {
    auto report = std::make_shared<MockPacketizerReport>();
    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create(report);
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
    auto report = std::make_shared<MockPacketizerReport>();
    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create(report);
    auto id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{"video/avc"});

    auto now = mcs::Utils::GetNowUs();
    auto buffer = mcs::video::Buffer::Create(sizeof(csd0));
    ::memcpy(buffer->Data(), csd0, sizeof(csd0));
    buffer->SetTimestamp(now);

    EXPECT_CALL(*report, PacketizedFrame(buffer->Timestamp()))
            .Times(1);

    mcs::video::Buffer::Ptr out;

    packetizer->Packetize(id, buffer, &out);

    MPEGTSPacketMatcher matcher(out);

    matcher.ExpectPackets(1);
    matcher.At(0).ExpectPackets(1);
    matcher.At(0).ExpectValid();
    matcher.At(0).ExpectPID(0x1011);
    matcher.At(0).ExpectPaddingBytesAndContinuityCounter(0);
    matcher.At(0).ExpectData(buffer->Data(), buffer->Length());

    EXPECT_GE(now, out->Timestamp());
}

TEST(MPEGTSPacketizer, EmitPCRandPATandPMT) {
    auto report = std::make_shared<MockPacketizerReport>();
    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create(report);
    auto id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{"video/avc"});

    mcs::video::Buffer::Ptr out;
    auto buffer = CreateFrame(100);

    EXPECT_CALL(*report, PacketizedFrame(buffer->Timestamp()))
            .Times(1);

    packetizer->Packetize(id, buffer, &out, mcs::streaming::Packetizer::kEmitPCR |
                          mcs::streaming::Packetizer::kEmitPATandPMT);

    MPEGTSPacketMatcher matcher(out);

    // We should have four packets now:
    // 1. PAT
    // 2. PMT
    // 3. PCR
    // 4. Actual TS packet
    matcher.ExpectPackets(4);

    matcher.At(0).ExpectValid();
    matcher.At(0).ExpectPID(0);
    matcher.At(0).ExpectNoPaddingBytesAndContinuityCounter(1);

    matcher.At(1).ExpectValid();
    matcher.At(1).ExpectPID(0x100);
    matcher.At(1).ExpectNoPaddingBytesAndContinuityCounter(1);

    matcher.At(2).ExpectValid();
    matcher.At(2).ExpectPID(0x1000);
    // It doesn't really set a continuity counter for the PCR
    // TS packet so just compare the static value we expect here.
    matcher.At(2).ExpectByte(3, 0x20);

    matcher.At(3).ExpectValid();
    matcher.At(3).ExpectPID(0x1011);
    matcher.At(3).ExpectPaddingBytesAndContinuityCounter(0);
    matcher.At(3).ExpectData(buffer->Data(), buffer->Length());

    EXPECT_GE(0, out->Timestamp());
}

TEST(MPEGTSPacketizer, IncreasingContinuityCounter) {
    auto report = std::make_shared<MockPacketizerReport>();
    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create(report);
    auto id = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{"video/avc"});

    EXPECT_CALL(*report, PacketizedFrame(_))
            .Times(20);

    // Make sure we looper over 15 here as that is used for the continuity counter
    // for PAT/PMT and PCR but shouldn't be used here.
    for (int n = 0; n < 20; n++) {
        mcs::video::Buffer::Ptr out;
        auto buffer = CreateFrame(100);

        packetizer->Packetize(id, buffer, &out, mcs::streaming::Packetizer::kEmitPCR |
                              mcs::streaming::Packetizer::kEmitPATandPMT);

        MPEGTSPacketMatcher matcher(out);

        matcher.ExpectPackets(4);

        matcher.At(0).ExpectValid();
        matcher.At(0).ExpectPID(0);
        // Continuity counter starts a 1 and goes up to 15
        matcher.At(0).ExpectNoPaddingBytesAndContinuityCounter((n + 1) % 16);

        matcher.At(1).ExpectValid();
        matcher.At(1).ExpectPID(0x100);
        // Continuity counter starts a 1 and goes up to 15
        matcher.At(1).ExpectNoPaddingBytesAndContinuityCounter((n + 1) % 16);

        matcher.At(2).ExpectValid();
        matcher.At(2).ExpectPID(0x1000);
        // It doesn't really set a continuity counter for the PCR
        // TS packet so just compare the static value we expect here.
        matcher.At(2).ExpectByte(3, 0x20);

        matcher.At(3).ExpectValid();
        matcher.At(3).ExpectPID(0x1011);
        matcher.At(3).ExpectPaddingBytesAndContinuityCounter(n);
        matcher.At(3).ExpectData(buffer->Data(), buffer->Length());
    }
}
