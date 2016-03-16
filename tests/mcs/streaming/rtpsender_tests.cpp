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

#include "mcs/network/stream.h"

#include "mcs/streaming/rtpsender.h"

using namespace ::testing;

namespace {
static constexpr unsigned int kStreamMaxUnitSize = 1472;

class MockNetworkStream : public mcs::network::Stream {
public:
    MOCK_METHOD0(WaitUntilReady, bool());
    MOCK_METHOD2(Write, mcs::network::Stream::Error(const uint8_t*, unsigned int));
    MOCK_CONST_METHOD0(LocalPort, mcs::network::Port());
    MOCK_CONST_METHOD0(MaxUnitSize, std::uint32_t());
};

class MockSenderReport : public mcs::video::SenderReport {
public:
    MOCK_METHOD2(SentPacket, void(const mcs::TimestampUs&, const size_t&));
};
}

TEST(RTPSender, ForwardsCorrectPort) {
    auto mock_stream = std::make_shared<MockNetworkStream>();
    auto mock_report = std::make_shared<MockSenderReport>();

    EXPECT_CALL(*mock_stream, MaxUnitSize())
            .WillRepeatedly(Return(kStreamMaxUnitSize));

    mcs::network::Port local_port = 1234;

    EXPECT_CALL(*mock_stream, LocalPort())
            .Times(1)
            .WillOnce(Return(local_port));

    auto sender = std::make_shared<mcs::streaming::RTPSender>(mock_stream, mock_report);

    EXPECT_EQ(local_port, sender->LocalPort());
}

TEST(RTPSender, SpecifiesExecutableName) {
    auto mock_stream = std::make_shared<MockNetworkStream>();
    auto mock_report = std::make_shared<MockSenderReport>();

    EXPECT_CALL(*mock_stream, MaxUnitSize())
            .WillRepeatedly(Return(kStreamMaxUnitSize));

    auto sender = std::make_shared<mcs::streaming::RTPSender>(mock_stream, mock_report);
    EXPECT_NE(0, sender->Name().length());
}

TEST(RTPSender, StartAndStopDoesNotFail) {
    auto mock_stream = std::make_shared<MockNetworkStream>();
    auto mock_report = std::make_shared<MockSenderReport>();

    EXPECT_CALL(*mock_stream, MaxUnitSize())
            .WillRepeatedly(Return(kStreamMaxUnitSize));

    auto sender = std::make_shared<mcs::streaming::RTPSender>(mock_stream, mock_report);

    EXPECT_TRUE(sender->Start());
    EXPECT_TRUE(sender->Stop());
}

TEST(RTPSender, DoeNotAcceptIncorrectPacketSizes) {
    auto mock_stream = std::make_shared<MockNetworkStream>();
    auto mock_report = std::make_shared<MockSenderReport>();

    EXPECT_CALL(*mock_stream, MaxUnitSize())
            .WillRepeatedly(Return(kStreamMaxUnitSize));

    auto sender = std::make_shared<mcs::streaming::RTPSender>(mock_stream, mock_report);

    auto packets = mcs::video::Buffer::Create(188 + 1);
    EXPECT_FALSE(sender->Queue(packets));

    packets = mcs::video::Buffer::Create(188 - 1);
    EXPECT_FALSE(sender->Queue(packets));
}

TEST(RTPSender, ExecutesWithEmptyQueue) {
    auto mock_stream = std::make_shared<MockNetworkStream>();
    auto mock_report = std::make_shared<MockSenderReport>();

    EXPECT_CALL(*mock_stream, MaxUnitSize())
            .WillRepeatedly(Return(kStreamMaxUnitSize));

    auto sender = std::make_shared<mcs::streaming::RTPSender>(mock_stream, mock_report);

    EXPECT_TRUE(sender->Execute());
}

TEST(RTPSender, ExecuteDoesNotFailWhenStreamIsNotReady) {
    auto mock_stream = std::make_shared<MockNetworkStream>();
    auto mock_report = std::make_shared<MockSenderReport>();

    EXPECT_CALL(*mock_stream, MaxUnitSize())
            .WillRepeatedly(Return(kStreamMaxUnitSize));

    EXPECT_CALL(*mock_stream, WaitUntilReady())
            .Times(1)
            .WillOnce(Return(false));

    auto sender = std::make_shared<mcs::streaming::RTPSender>(mock_stream, mock_report);

    auto packets = mcs::video::Buffer::Create(188);

    EXPECT_TRUE(sender->Queue(packets));
    EXPECT_TRUE(sender->Execute());
}

TEST(RTPSender, QueuesUpPackagesAndSendsAll) {
    auto mock_stream = std::make_shared<MockNetworkStream>();
    auto mock_report = std::make_shared<MockSenderReport>();

    auto now = mcs::Utils::GetNowUs();

    EXPECT_CALL(*mock_report, SentPacket(now, _))
            .Times(3);

    EXPECT_CALL(*mock_stream, MaxUnitSize())
            .WillRepeatedly(Return(kStreamMaxUnitSize));

    EXPECT_CALL(*mock_stream, WaitUntilReady())
            .Times(1)
            .WillOnce(Return(true));

    EXPECT_CALL(*mock_stream, Write(_, _))
            .Times(3)
            .WillRepeatedly(Return(mcs::network::Stream::Error::kNone));

    auto sender = std::make_shared<mcs::streaming::RTPSender>(mock_stream, mock_report);

    auto packets = mcs::video::Buffer::Create(188 * 15);
    packets->SetTimestamp(now);

    EXPECT_TRUE(sender->Queue(packets));
    EXPECT_TRUE(sender->Execute());
}

TEST(RTPSender, WritePackageFails) {
    auto mock_stream = std::make_shared<MockNetworkStream>();
    auto mock_report = std::make_shared<MockSenderReport>();

    EXPECT_CALL(*mock_stream, MaxUnitSize())
            .WillRepeatedly(Return(kStreamMaxUnitSize));

    EXPECT_CALL(*mock_stream, WaitUntilReady())
            .Times(1)
            .WillOnce(Return(true));

    EXPECT_CALL(*mock_stream, Write(_, _))
            .Times(1)
            .WillOnce(Return(mcs::network::Stream::Error::kRemoteClosedConnection));

    auto sender = std::make_shared<mcs::streaming::RTPSender>(mock_stream, mock_report);

    auto packets = mcs::video::Buffer::Create(188);

    EXPECT_TRUE(sender->Queue(packets));
    EXPECT_FALSE(sender->Execute());
}
