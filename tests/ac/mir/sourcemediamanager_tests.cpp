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

#include "tests/common/glibhelpers.h"

#include "ac/mir/sourcemediamanager.h"

using namespace ::testing;

namespace {
class MockOutputStream : public ac::network::Stream {
public:
    MOCK_METHOD2(Connect, bool(const std::string&, const ac::network::Port&));
    MOCK_METHOD0(WaitUntilReady, bool());
    MOCK_METHOD3(Write, ac::network::Stream::Error(const uint8_t*, unsigned int, const ac::TimestampUs&));
    MOCK_CONST_METHOD0(LocalPort, ac::network::Port());
    MOCK_CONST_METHOD0(MaxUnitSize, std::uint32_t());
};

class MockBufferProducer : public ac::video::BufferProducer {
public:
    MOCK_METHOD1(Setup, bool(const ac::video::DisplayOutput&));
    MOCK_METHOD0(SwapBuffers, void());
    MOCK_CONST_METHOD0(CurrentBuffer, void*());
    MOCK_CONST_METHOD0(OutputMode, ac::video::DisplayOutput());
};

class MockEncoder : public ac::video::BaseEncoder {
public:
    MOCK_METHOD0(DefaultConfiguration, ac::video::BaseEncoder::Config());
    MOCK_METHOD1(Configure, bool(const ac::video::BaseEncoder::Config&));
    MOCK_METHOD1(QueueBuffer, void(const ac::video::Buffer::Ptr&));
    MOCK_CONST_METHOD0(Configuration, ac::video::BaseEncoder::Config());
    MOCK_CONST_METHOD0(Running, bool());
    MOCK_METHOD0(SendIDRFrame, void());
    MOCK_CONST_METHOD0(Name, std::string());
    MOCK_METHOD0(Start, bool());
    MOCK_METHOD0(Stop, bool());
    MOCK_METHOD0(Execute, bool());
};

class MockReportFactory : public ac::report::ReportFactory {
public:
    MOCK_METHOD0(CreateEncoderReport, ac::video::EncoderReport::Ptr());
    MOCK_METHOD0(CreateRendererReport, ac::video::RendererReport::Ptr());
    MOCK_METHOD0(CreatePacketizerReport, ac::video::PacketizerReport::Ptr());
    MOCK_METHOD0(CreateSenderReport, ac::video::SenderReport::Ptr());
};

class MockExecutorFactory : public ac::common::ExecutorFactory {
public:
    MOCK_METHOD1(Create, ac::common::Executor::Ptr(const ac::common::Executable::Ptr&));
};

class MockExecutor : public ac::common::Executor {
public:
    MOCK_METHOD0(Start, bool());
    MOCK_METHOD0(Stop, bool());
    MOCK_CONST_METHOD0(Running, bool());
};

class SourceMediaManagerFixture : public Test {
public:
    bool Configure(const ac::mir::SourceMediaManager::Ptr &manager) {
        std::vector<wds::H264VideoCodec> sink_supported_codecs;
        wds::RateAndResolutionsBitmap cea_rr;
        wds::RateAndResolutionsBitmap vesa_rr;
        wds::RateAndResolutionsBitmap hh_rr;

        cea_rr.set(wds::CEA1280x720p30);

        wds::H264VideoCodec codec(wds::CBP, wds::k3_2, cea_rr, vesa_rr, hh_rr);
        sink_supported_codecs.push_back(codec);

        wds::NativeVideoFormat sink_native_format;
        sink_native_format.type = wds::CEA;
        sink_native_format.rate_resolution = wds::CEA1280x720p30;

        return manager->InitOptimalVideoFormat(sink_native_format, sink_supported_codecs);
    }

    void ExpectCorrectConfiguration() {
        EXPECT_CALL(*mock_executor_factory, Create(_))
                .Times(4)
                .WillRepeatedly(Return(mock_executor));

        EXPECT_CALL(*mock_output_stream, Connect(remote_address, _))
                .WillOnce(Return(true));

        EXPECT_CALL(*mock_output_stream, MaxUnitSize())
                .WillOnce(Return(1000));

        EXPECT_CALL(*mock_buffer_producer, Setup(_))
                .WillOnce(Return(true));

        EXPECT_CALL(*mock_buffer_producer, OutputMode())
                .WillRepeatedly(Return(ac::video::DisplayOutput{}));

        EXPECT_CALL(*mock_encoder, DefaultConfiguration())
                .WillOnce(Return(ac::video::BaseEncoder::Config{}));

        EXPECT_CALL(*mock_encoder, Configure(_))
                .WillOnce(Return(true));

        EXPECT_CALL(*mock_encoder, Configuration())
                .WillOnce(Return(ac::video::BaseEncoder::Config{}));

        EXPECT_CALL(*mock_report_factory, CreateRendererReport())
                .WillOnce(Return(nullptr));

        EXPECT_CALL(*mock_report_factory, CreateSenderReport())
                .WillOnce(Return(nullptr));

        EXPECT_CALL(*mock_report_factory, CreatePacketizerReport())
                .WillOnce(Return(nullptr));
    }

    std::string remote_address = "127.0.0.1";
    std::shared_ptr<MockExecutor> mock_executor = std::make_shared<MockExecutor>();
    std::shared_ptr<MockExecutorFactory> mock_executor_factory = std::make_shared<MockExecutorFactory>();
    std::shared_ptr<MockBufferProducer> mock_buffer_producer = std::make_shared<MockBufferProducer>();
    std::shared_ptr<MockOutputStream> mock_output_stream = std::make_shared<MockOutputStream>();
    std::shared_ptr<MockEncoder> mock_encoder = std::make_shared<MockEncoder>();
    std::shared_ptr<MockReportFactory> mock_report_factory = std::make_shared<MockReportFactory>();
};
}

TEST_F(SourceMediaManagerFixture, FailingStreamCausesConfigureToFail) {
    EXPECT_CALL(*mock_output_stream, Connect(remote_address, _))
            .WillOnce(Return(false));

    const auto manager = std::make_shared<ac::mir::SourceMediaManager>(
                remote_address,
                mock_executor_factory,
                nullptr,
                nullptr,
                mock_output_stream,
                nullptr);

    EXPECT_FALSE(Configure(manager));
}

TEST_F(SourceMediaManagerFixture, ConfiguresFailsWithFailingProducer) {
    EXPECT_CALL(*mock_output_stream, Connect(remote_address, _))
            .WillOnce(Return(true));

    EXPECT_CALL(*mock_buffer_producer, Setup(_))
            .WillOnce(Return(false));

    const auto manager = std::make_shared<ac::mir::SourceMediaManager>(
                remote_address,
                mock_executor_factory,
                mock_buffer_producer,
                nullptr,
                mock_output_stream,
                nullptr);

    EXPECT_FALSE(Configure(manager));
}

TEST_F(SourceMediaManagerFixture, ConfigureFailsWithFailingEncoder) {
    EXPECT_CALL(*mock_output_stream, Connect(remote_address, _))
            .WillOnce(Return(true));

    EXPECT_CALL(*mock_buffer_producer, Setup(_))
            .WillOnce(Return(true));

    EXPECT_CALL(*mock_encoder, DefaultConfiguration())
            .WillOnce(Return(ac::video::BaseEncoder::Config{}));

    EXPECT_CALL(*mock_encoder, Configure(_))
            .WillOnce(Return(false));

    const auto manager = std::make_shared<ac::mir::SourceMediaManager>(
                remote_address,
                mock_executor_factory,
                mock_buffer_producer,
                mock_encoder,
                mock_output_stream,
                nullptr);

    EXPECT_FALSE(Configure(manager));
}

TEST_F(SourceMediaManagerFixture, ConfiugresAndSetsUpEverything) {
    ExpectCorrectConfiguration();

    const auto manager = std::make_shared<ac::mir::SourceMediaManager>(
                remote_address,
                mock_executor_factory,
                mock_buffer_producer,
                mock_encoder,
                mock_output_stream,
                mock_report_factory);

    EXPECT_TRUE(Configure(manager));
}

TEST_F(SourceMediaManagerFixture, StateSwitching) {
    ExpectCorrectConfiguration();

    EXPECT_CALL(*mock_executor, Start())
            .Times(8)
            .WillRepeatedly(Return(true));

    EXPECT_CALL(*mock_executor, Stop())
            .Times(8)
            .WillRepeatedly(Return(true));

    const auto manager = std::make_shared<ac::mir::SourceMediaManager>(
                remote_address,
                mock_executor_factory,
                mock_buffer_producer,
                mock_encoder,
                mock_output_stream,
                mock_report_factory);

    EXPECT_TRUE(Configure(manager));

    EXPECT_TRUE(manager->IsPaused());

    manager->Play();
    ac::testing::RunMainLoop(std::chrono::seconds{1});
    EXPECT_FALSE(manager->IsPaused());

    manager->Pause();
    EXPECT_TRUE(manager->IsPaused());

    manager->Play();
    ac::testing::RunMainLoop(std::chrono::seconds{1});
    EXPECT_FALSE(manager->IsPaused());

    manager->Teardown();
    EXPECT_TRUE(manager->IsPaused());
}

TEST_F(SourceMediaManagerFixture, SendsIDRPicture) {
    const auto manager = std::make_shared<ac::mir::SourceMediaManager>(
                remote_address,
                mock_executor_factory,
                mock_buffer_producer,
                mock_encoder,
                mock_output_stream,
                mock_report_factory);

    EXPECT_CALL(*mock_encoder, SendIDRFrame())
            .Times(1);

    manager->SendIDRPicture();
}
