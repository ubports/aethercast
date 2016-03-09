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

#include "mcs/streaming/mediasender.h"

using namespace ::testing;

namespace {
class MockTransportSender : public mcs::streaming::TransportSender {
public:
    MOCK_METHOD1(Queue, bool(const mcs::video::Buffer::Ptr&));
    MOCK_CONST_METHOD0(LocalPort, int32_t());
};

class MockPacketizer : public mcs::streaming::Packetizer {
public:
    MOCK_METHOD1(AddTrack, TrackId(const TrackFormat&));
    MOCK_METHOD2(SubmitCSD, void(TrackId, const mcs::video::Buffer::Ptr&));
    MOCK_METHOD4(Packetize, bool(TrackId, const mcs::video::Buffer::Ptr&,
                                 mcs::video::Buffer::Ptr*, int));
};
}

TEST(MediaSender, WitNothingAndNoCrash) {
    auto encoder_config = mcs::video::BaseEncoder::Config{};
    auto sender = mcs::streaming::MediaSender::Create(nullptr, nullptr, encoder_config);

    sender->Start();
    sender->Stop();
    EXPECT_EQ(0, sender->LocalRTPPort());
    sender->OnBufferAvailable(mcs::video::Buffer::Create(nullptr));
    sender->OnBufferReturned();
    sender->OnBufferWithCodecConfig(mcs::video::Buffer::Create(nullptr));
}

TEST(MediaSender, CreateCorrectVideoTrack) {
    auto encoder_config = mcs::video::BaseEncoder::Config{};
    encoder_config.profile_idc = 1;
    encoder_config.level_idc = 2;
    encoder_config.constraint_set = 3;

    auto dummy_packetizer = std::make_shared<MockPacketizer>();
    auto dummy_transport = std::make_shared<MockTransportSender>();

    auto track_format = mcs::streaming::Packetizer::TrackFormat{"video/avc", 1, 2, 3};

    EXPECT_CALL(*dummy_packetizer, AddTrack(track_format))
            .Times(1)
            .WillRepeatedly(Return(1));

    auto sender = mcs::streaming::MediaSender::Create(dummy_packetizer, dummy_transport, encoder_config);
}

TEST(MediaSender, BufferGetsPacketizedAndSend) {
    auto encoder_config = mcs::video::BaseEncoder::Config{};

    auto dummy_packetizer = std::make_shared<MockPacketizer>();
    auto dummy_transport = std::make_shared<MockTransportSender>();

    auto now = mcs::Utils::GetNowUs();
    auto buffer = mcs::video::Buffer::Create(1, now);
    auto packets = mcs::video::Buffer::Create(10);

    EXPECT_CALL(*dummy_packetizer, AddTrack(_))
            .Times(1)
            .WillRepeatedly(Return(1));
    EXPECT_CALL(*dummy_packetizer, Packetize(1, buffer, NotNull(), _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<2>(packets), Return(true)));
    EXPECT_CALL(*dummy_transport, Queue(packets))
            .Times(1)
            .WillRepeatedly(Return(true));

    auto sender = mcs::streaming::MediaSender::Create(dummy_packetizer, dummy_transport, encoder_config);

    sender->Start();

    sender->OnBufferAvailable(buffer);

    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    EXPECT_EQ(now, packets->Timestamp());

    sender->Stop();
}

TEST(MediaSender, BufferPacketizingFails) {
    auto encoder_config = mcs::video::BaseEncoder::Config{};

    auto dummy_packetizer = std::make_shared<MockPacketizer>();
    auto dummy_transport = std::make_shared<MockTransportSender>();

    auto buffer = mcs::video::Buffer::Create(1);

    EXPECT_CALL(*dummy_packetizer, AddTrack(_))
            .Times(1)
            .WillRepeatedly(Return(1));
    EXPECT_CALL(*dummy_packetizer, Packetize(1, buffer, NotNull(), _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<2>(nullptr), Return(false)));
    EXPECT_CALL(*dummy_transport, Queue(_))
            .Times(0);

    auto sender = mcs::streaming::MediaSender::Create(dummy_packetizer, dummy_transport, encoder_config);

    sender->Start();

    sender->OnBufferAvailable(buffer);

    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    sender->Stop();
}

TEST(MediaSender, BufferWithCodecConfig) {
    auto encoder_config = mcs::video::BaseEncoder::Config{};

    auto dummy_packetizer = std::make_shared<MockPacketizer>();
    auto dummy_transport = std::make_shared<MockTransportSender>();

    auto csd_buffer = mcs::video::Buffer::Create(1);

    EXPECT_CALL(*dummy_packetizer, AddTrack(_))
            .Times(1)
            .WillRepeatedly(Return(1));

    // Buffers which contain codec configuration data will be submitted twice. First
    // through OnBufferWithCodecConfig and then a second time through OnBufferWithCodecConfig.

    EXPECT_CALL(*dummy_packetizer, SubmitCSD(1, csd_buffer))
            .Times(1);
    EXPECT_CALL(*dummy_packetizer, Packetize(_, _, _, _))
            .Times(0);
    EXPECT_CALL(*dummy_transport, Queue(_))
            .Times(0);

    auto sender = mcs::streaming::MediaSender::Create(dummy_packetizer, dummy_transport, encoder_config);

    sender->Start();

    sender->OnBufferWithCodecConfig(csd_buffer);

    std::this_thread::sleep_for(std::chrono::milliseconds{10});

    sender->Stop();
}

TEST(MediaSender, RequestsPCRandPATandPMTEvery100ms) {
    auto encoder_config = mcs::video::BaseEncoder::Config{};

    auto dummy_packetizer = std::make_shared<MockPacketizer>();
    auto dummy_transport = std::make_shared<MockTransportSender>();

    auto buffer = mcs::video::Buffer::Create(1);
    auto packets = mcs::video::Buffer::Create(10);

    auto expected_flags = mcs::streaming::Packetizer::kEmitPATandPMT |
            mcs::streaming::Packetizer::kEmitPCR;

    EXPECT_CALL(*dummy_packetizer, AddTrack(_))
            .Times(1)
            .WillRepeatedly(Return(1));

    EXPECT_CALL(*dummy_packetizer, Packetize(1, buffer, NotNull(), expected_flags))
            .Times(2)
            .WillRepeatedly(DoAll(SetArgPointee<2>(packets), Return(true)));

    EXPECT_CALL(*dummy_packetizer, Packetize(1, buffer, NotNull(), 0))
            .Times(2)
            .WillRepeatedly(DoAll(SetArgPointee<2>(packets), Return(true)));

    EXPECT_CALL(*dummy_transport, Queue(_))
            .Times(4);

    auto sender = mcs::streaming::MediaSender::Create(dummy_packetizer, dummy_transport, encoder_config);

    sender->Start();

    // As this is the first buffer the sender will ask packetizer to include
    // PCR / PAT and PMT
    sender->OnBufferAvailable(buffer);

    std::this_thread::sleep_for(std::chrono::milliseconds{5});

    // Second one shouldn't include PCR / PAT and PMT
    sender->OnBufferAvailable(buffer);

    std::this_thread::sleep_for(std::chrono::milliseconds{5});

    // As 100ms later this will include both PCR / PAT and PMT
    sender->OnBufferAvailable(buffer);

    // PCR / PAT and PMT will be included every 100ms so wait a bit until the sender
    // will do that.
    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    // As this buffer is send directly after the previous one which include
    // both PCR / PAT and PMT this wont get them attached.
    sender->OnBufferAvailable(buffer);

    std::this_thread::sleep_for(std::chrono::milliseconds{5});

    sender->Stop();
}
