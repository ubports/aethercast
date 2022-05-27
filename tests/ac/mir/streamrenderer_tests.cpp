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

#include <atomic>

#include "ac/mir/streamrenderer.h"

using namespace ::testing;

namespace {
class MockBufferProducer : public ac::video::BufferProducer {
public:
    MOCK_METHOD1(Setup, bool(const ac::video::DisplayOutput&));
    MOCK_METHOD0(Stop, bool());
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

class MockRendererReport : public ac::video::RendererReport {
public:
    MOCK_METHOD0(BeganFrame, void());
    MOCK_METHOD1(FinishedFrame, void(const ac::TimestampUs&));
};

class StreamRendererFixture : public ::testing::Test {
public:
    StreamRendererFixture() :
        mock_buffer_producer(std::make_shared<MockBufferProducer>()),
        mock_encoder(std::make_shared<MockEncoder>()),
        mock_renderer_report(std::make_shared<MockRendererReport>()) {
    }

    void ExpectValidConfiguration() {
        ac::video::DisplayOutput output_mode{ac::video::DisplayOutput::Mode::kExtend, 1280, 720, 30};

        EXPECT_CALL(*mock_buffer_producer, OutputMode())
                .WillRepeatedly(Return(output_mode));

        ac::video::BaseEncoder::Config encoder_config{};
        // Need to set a framerate here as otherwise the renderer will sleep
        // forever when we call its Execute method.
        encoder_config.framerate = 30;

        EXPECT_CALL(*mock_encoder, Configuration())
                .WillRepeatedly(Return(encoder_config));
    }

    std::shared_ptr<MockBufferProducer> mock_buffer_producer;
    std::shared_ptr<MockEncoder> mock_encoder;
    std::shared_ptr<MockRendererReport> mock_renderer_report;
};
}

TEST_F(StreamRendererFixture, ValidExecutableName) {
    ExpectValidConfiguration();

    const auto renderer = std::make_shared<ac::mir::StreamRenderer>(
                mock_buffer_producer,
                mock_encoder,
                mock_renderer_report);

    EXPECT_NE(0, renderer->Name().length());
}

TEST_F(StreamRendererFixture, StartAndStopSucceed) {
    ExpectValidConfiguration();

    const auto renderer = std::make_shared<ac::mir::StreamRenderer>(
                mock_buffer_producer,
                mock_encoder,
                mock_renderer_report);

    EXPECT_TRUE(renderer->Start());
    EXPECT_TRUE(renderer->Stop());
}

TEST_F(StreamRendererFixture, TakesBufferFromProducerAndForwardsToEncoder) {
    ExpectValidConfiguration();

    const auto renderer = std::make_shared<ac::mir::StreamRenderer>(
                mock_buffer_producer,
                mock_encoder,
                mock_renderer_report);

    EXPECT_TRUE(renderer->Start());

    EXPECT_CALL(*mock_renderer_report, BeganFrame())
            .Times(1);

    EXPECT_CALL(*mock_renderer_report, FinishedFrame(_))
            .Times(1);

    auto buffer_native_handle = reinterpret_cast<void*>(1);

    EXPECT_CALL(*mock_buffer_producer, SwapBuffers())
            .Times(1);

    EXPECT_CALL(*mock_buffer_producer, CurrentBuffer())
            .WillOnce(Return(buffer_native_handle));

    ac::video::Buffer::Ptr output_buffer;

    EXPECT_CALL(*mock_encoder, QueueBuffer(_))
            .WillOnce(SaveArg<0>(&output_buffer));

    EXPECT_TRUE(renderer->Execute());

    EXPECT_TRUE(renderer->Stop());

    EXPECT_NE(nullptr, output_buffer.get());
    EXPECT_LE(0, output_buffer->Timestamp());
    EXPECT_EQ(buffer_native_handle, output_buffer->NativeHandle());
}

TEST_F(StreamRendererFixture, CorrectBufferManagement) {
    ExpectValidConfiguration();

    const auto renderer = std::make_shared<ac::mir::StreamRenderer>(
                mock_buffer_producer,
                mock_encoder,
                mock_renderer_report);

    std::atomic<bool> running;
    std::atomic<std::uint32_t> iteration_count;
    auto buffers = ac::video::BufferQueue::Create(renderer->BufferSlots());

    running = true;
    iteration_count = 0;

    EXPECT_CALL(*mock_encoder, QueueBuffer(_))
            .WillRepeatedly(Invoke([&](const ac::video::Buffer::Ptr &buffer) {
                buffers->Push(buffer);
            }));

    EXPECT_CALL(*mock_renderer_report, BeganFrame())
            .Times(AtLeast(1));

    EXPECT_CALL(*mock_renderer_report, FinishedFrame(_))
            .Times(AtLeast(1));

    EXPECT_CALL(*mock_buffer_producer, SwapBuffers())
            .Times(AtLeast(1));

    auto buffer_native_handle = reinterpret_cast<uint8_t*>(1);

    EXPECT_CALL(*mock_buffer_producer, CurrentBuffer())
            .WillRepeatedly(Return(reinterpret_cast<void*>(buffer_native_handle++)));

    auto thread_renderer = std::thread([&]() {
        while (running && renderer->Execute()) {
            iteration_count++;
            if (iteration_count > 10) {
                running.exchange(false);
                break;
            }
        }
    });

    auto thread_consumer = std::thread([&]() {
        while (running) {
            if (!buffers->WaitToBeFilled())
                continue;
            auto finished_buffer = buffers->Pop();
            renderer->OnBufferFinished(finished_buffer);
        }
    });

    thread_renderer.join();
    thread_consumer.join();
}

TEST_F(StreamRendererFixture, KeepsExecutingWhenNoFreeSlots) {
    ExpectValidConfiguration();

    const auto renderer = std::make_shared<ac::mir::StreamRenderer>(
                mock_buffer_producer,
                mock_encoder,
                mock_renderer_report);

    auto buffers = ac::video::BufferQueue::Create(renderer->BufferSlots());

    EXPECT_CALL(*mock_encoder, QueueBuffer(_))
            .WillRepeatedly(Invoke([&](const ac::video::Buffer::Ptr &buffer) {
                buffers->Push(buffer);
            }));

    EXPECT_CALL(*mock_renderer_report, BeganFrame())
            .Times(2);

    EXPECT_CALL(*mock_renderer_report, FinishedFrame(_))
            .Times(2);

    EXPECT_CALL(*mock_buffer_producer, SwapBuffers())
            .Times(2);

    auto buffer_native_handle = reinterpret_cast<uint8_t*>(1);

    EXPECT_CALL(*mock_buffer_producer, CurrentBuffer())
            .Times(2)
            .WillRepeatedly(Return(reinterpret_cast<void*>(buffer_native_handle++)));

    EXPECT_TRUE(renderer->Start());

    for (int n = 0; n < 10; n++)
        EXPECT_TRUE(renderer->Execute());

    EXPECT_TRUE(renderer->Stop());

    EXPECT_EQ(2, buffers->Size());
}
