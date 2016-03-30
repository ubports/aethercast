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

// Ignore all warnings coming from the external Android headers as
// we don't control them and also don't want to get any warnings
// from them which will only pollute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include <system/window.h>
#pragma GCC diagnostic pop

#include "mcs/report/reportfactory.h"

#include "mcs/android/h264encoder.h"

#include "mockmedia.h"
#include "mockencoderreport.h"

using namespace ::testing;

namespace {
// From frameworks/native/include/media/openmax/OMX_IVCommon.h
const int32_t kOMXColorFormatAndroidOpaque = 0x7F000789;
const int32_t kOMXVideoIntraRefreshCyclic = 0;
// From frameworks/native/include/media/openmax/OMX_Video.h
const int32_t kOMXVideoControlRateConstant = 2;
// From frameworks/native/include/media/hardware/MetadataBufferType.h
const uint32_t kMetadataBufferTypeGrallocSource = 1;

class MockEncoderDelegate : public mcs::video::BaseEncoder::Delegate {
public:
    MOCK_METHOD1(OnBufferAvailable, void(const mcs::video::Buffer::Ptr&));
    MOCK_METHOD1(OnBufferWithCodecConfig, void(const mcs::video::Buffer::Ptr&));
    MOCK_METHOD0(OnBufferReturned, void());
};

class MockBufferDelegate : public mcs::video::Buffer::Delegate {
public:
    MOCK_METHOD1(OnBufferFinished, void(const mcs::video::Buffer::Ptr&));
};

struct DummyMediaMessageWrapper {
};

struct DummyMediaSourceWrapper {
};

struct DummyMediaMetaDataWrapper {
};

struct DummyMediaCodecSource {
};

struct DummyMediaBufferWrapper {
};

class H264EncoderFixture : public ::testing::Test {
public:
    H264EncoderFixture() :
        mock_report(std::make_shared<mcs::test::android::MockEncoderReport>()) {
    }

    void ExpectValidConfiguration(const mcs::video::BaseEncoder::Config &config, const mcs::test::android::MockMedia::Ptr &mock) {
        auto message = new DummyMediaMessageWrapper;

        EXPECT_CALL(*mock, media_message_create())
                .Times(1)
                .WillRepeatedly(Return(message));
        EXPECT_CALL(*mock, media_message_release(message))
                .Times(1)
                .WillOnce(Invoke([](MediaMessageWrapper *msg) { delete msg; }));
        EXPECT_CALL(*mock, media_message_set_string(message, _, _, _))
                .Times(AtLeast(0));
        EXPECT_CALL(*mock, media_message_set_int32(message, _, _))
                .Times(AtLeast(0));

        auto source = new DummyMediaSourceWrapper;

        EXPECT_CALL(*mock, media_source_create())
                .Times(1)
                .WillRepeatedly(Return(source));
        EXPECT_CALL(*mock, media_source_release(source))
                .Times(1)
                .WillOnce(Invoke([](MediaSourceWrapper *source) { delete source; }));

        auto meta_data = new DummyMediaMetaDataWrapper;

        EXPECT_CALL(*mock, media_meta_data_create())
                .Times(1)
                .WillRepeatedly(Return(meta_data));
        EXPECT_CALL(*mock, media_meta_data_release(meta_data))
                .Times(1)
                .WillOnce(Invoke([](MediaMetaDataWrapper *meta) { delete meta; }));
        EXPECT_CALL(*mock, media_meta_data_set_cstring(meta_data, _, _))
                .Times(AtLeast(0));
        EXPECT_CALL(*mock, media_meta_data_set_int32(meta_data, _, _))
                .Times(AtLeast(0));

        EXPECT_CALL(*mock, media_meta_data_get_key_id(_))
                .Times(AtLeast(0))
                .WillRepeatedly(Return(0));

        EXPECT_CALL(*mock, media_source_set_format(source, meta_data))
                .Times(1);
        EXPECT_CALL(*mock, media_source_set_start_callback(source, _, _))
                .Times(1);
        EXPECT_CALL(*mock, media_source_set_stop_callback(source, _, _))
                .Times(1);
        EXPECT_CALL(*mock, media_source_set_pause_callback(source, _, _))
                .Times(1);
        EXPECT_CALL(*mock, media_source_set_read_callback(source, _, _))
                .Times(1)
                .WillRepeatedly(DoAll(SaveArg<1>(&source_read_callback),
                                      SaveArg<2>(&source_read_callback_data)));

        auto codec_source = new DummyMediaCodecSource;

        EXPECT_CALL(*mock, media_codec_source_create(message, source, _))
                .Times(1)
                .WillRepeatedly(Return(codec_source));
        EXPECT_CALL(*mock, media_codec_source_release(codec_source))
                .Times(1)
                .WillOnce(Invoke([](MediaCodecSourceWrapper *source) { delete source; }));
    }

    void ExpectValidStartAndStop(const mcs::test::android::MockMedia::Ptr &mock) {
        EXPECT_CALL(*mock, media_codec_source_start(_))
                .Times(1)
                .WillRepeatedly(Return(true));

        EXPECT_CALL(*mock_report, Started())
                .Times(1);

        EXPECT_CALL(*mock, media_codec_source_stop(_))
                .Times(1)
                .WillRepeatedly(Return(true));

        EXPECT_CALL(*mock_report, Stopped())
                .Times(1);
    }

    std::shared_ptr<mcs::test::android::MockEncoderReport> mock_report;

    MediaSourceReadCallback source_read_callback;
    void *source_read_callback_data;
};
}

TEST_F(H264EncoderFixture, ValidDefaultConfiguration) {
    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();
    EXPECT_EQ(-1, config.framerate);
    EXPECT_EQ(5000000, config.bitrate);
    EXPECT_EQ(15, config.i_frame_interval);
    EXPECT_EQ(0, config.intra_refresh_mode);
    EXPECT_EQ(0, config.width);
    EXPECT_EQ(0, config.height);
    EXPECT_EQ(0, config.profile);
    EXPECT_EQ(0, config.profile_idc);
    EXPECT_EQ(0, config.level);
    EXPECT_EQ(0, config.level_idc);
    EXPECT_EQ(0, config.constraint_set);
}

TEST_F(H264EncoderFixture, MediaMessageCreationFails) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    EXPECT_CALL(*mock, media_message_create())
            .Times(1)
            .WillRepeatedly(Return(nullptr));

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    EXPECT_FALSE(encoder->Configure(config));
}

TEST_F(H264EncoderFixture, MediaSourceCreationFails) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto message = new DummyMediaMessageWrapper;

    EXPECT_CALL(*mock, media_message_create())
            .Times(1)
            .WillRepeatedly(Return(message));
    EXPECT_CALL(*mock, media_message_release(message))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_string(message, _, _, _))
            .Times(AtLeast(0));
    EXPECT_CALL(*mock, media_message_set_int32(message, _, _))
            .Times(AtLeast(0));

    EXPECT_CALL(*mock, media_source_create())
            .Times(1)
            .WillRepeatedly(Return(nullptr));

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    EXPECT_FALSE(encoder->Configure(config));
}

TEST_F(H264EncoderFixture, MediaMetaDataCreationFails) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto message = new DummyMediaMessageWrapper;

    EXPECT_CALL(*mock, media_message_create())
            .Times(1)
            .WillRepeatedly(Return(message));
    EXPECT_CALL(*mock, media_message_release(message))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_string(message, _, _, _))
            .Times(AtLeast(0));
    EXPECT_CALL(*mock, media_message_set_int32(message, _, _))
            .Times(AtLeast(0));

    auto source = new DummyMediaSourceWrapper;

    EXPECT_CALL(*mock, media_source_create())
            .Times(1)
            .WillRepeatedly(Return(source));
    EXPECT_CALL(*mock, media_source_release(source))
            .Times(1);

    EXPECT_CALL(*mock, media_meta_data_create())
            .Times(1)
            .WillRepeatedly(Return(nullptr));

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    EXPECT_FALSE(encoder->Configure(config));
}

TEST_F(H264EncoderFixture, MediaCodecSourceCreationFails) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto message = new DummyMediaMessageWrapper;

    EXPECT_CALL(*mock, media_message_create())
            .Times(1)
            .WillRepeatedly(Return(message));
    EXPECT_CALL(*mock, media_message_release(message))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_string(message, _, _, _))
            .Times(AtLeast(0));
    EXPECT_CALL(*mock, media_message_set_int32(message, _, _))
            .Times(AtLeast(0));

    auto source = new DummyMediaSourceWrapper;

    EXPECT_CALL(*mock, media_source_create())
            .Times(1)
            .WillRepeatedly(Return(source));
    EXPECT_CALL(*mock, media_source_release(source))
            .Times(1);

    auto meta_data = new DummyMediaMetaDataWrapper;

    EXPECT_CALL(*mock, media_meta_data_create())
            .Times(1)
            .WillRepeatedly(Return(meta_data));
    EXPECT_CALL(*mock, media_meta_data_release(meta_data))
            .Times(1);
    EXPECT_CALL(*mock, media_meta_data_set_cstring(meta_data, _, _))
            .Times(AtLeast(0));
    EXPECT_CALL(*mock, media_meta_data_set_int32(meta_data, _, _))
            .Times(AtLeast(0));

    EXPECT_CALL(*mock, media_meta_data_get_key_id(_))
            .Times(AtLeast(0))
            .WillRepeatedly(Return(0));

    EXPECT_CALL(*mock, media_source_set_format(source, meta_data))
            .Times(1);
    EXPECT_CALL(*mock, media_source_set_start_callback(source, _, _))
            .Times(1);
    EXPECT_CALL(*mock, media_source_set_stop_callback(source, _, _))
            .Times(1);
    EXPECT_CALL(*mock, media_source_set_pause_callback(source, _, _))
            .Times(1);
    EXPECT_CALL(*mock, media_source_set_read_callback(source, _, _))
            .Times(1);

    EXPECT_CALL(*mock, media_codec_source_create(message, source, _))
            .Times(1)
            .WillRepeatedly(Return(nullptr));

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    EXPECT_FALSE(encoder->Configure(config));
}

TEST_F(H264EncoderFixture, CorrectConfiguration) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    auto config = encoder->DefaultConfiguration();
    config.width = 1280;
    config.height = 720;
    config.framerate = 30;
    config.profile_idc = 1;
    config.level_idc = 2;
    config.constraint_set = 3;

    auto format_message = new DummyMediaMessageWrapper;

    EXPECT_CALL(*mock, media_message_create())
            .Times(1)
            .WillRepeatedly(Return(format_message));
    EXPECT_CALL(*mock, media_message_release(format_message))
            .Times(1)
            .WillOnce(Invoke([](MediaMessageWrapper *msg) { delete msg; }));

    EXPECT_CALL(*mock, media_message_set_string(format_message, StrEq("mime"), StrEq("video/avc"), 0))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("store-metadata-in-buffers"), 1))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("store-metadata-in-buffers-output"), 0))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("width"), config.width))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("height"), config.height))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("stride"), config.width))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("slice-height"), config.width))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("color-format"), kOMXColorFormatAndroidOpaque))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("bitrate"), config.bitrate))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("bitrate-mode"), kOMXVideoControlRateConstant))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("frame-rate"), config.framerate))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("intra-refresh-mode"), config.intra_refresh_mode))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("intra-refresh-CIR-mbs"), 360))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("i-frame-interval"), config.i_frame_interval))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("prepend-sps-pps-to-idr-frames"), 1))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("profile-idc"), 1))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("level-idc"), 2))
            .Times(1);
    EXPECT_CALL(*mock, media_message_set_int32(format_message, StrEq("constraint-set"), 3))
            .Times(1);

    auto meta_data = new DummyMediaMetaDataWrapper;

    EXPECT_CALL(*mock, media_meta_data_create())
            .Times(1)
            .WillRepeatedly(Return(meta_data));
    EXPECT_CALL(*mock, media_meta_data_release(meta_data))
            .Times(1)
            .WillRepeatedly(Invoke([](MediaMetaDataWrapper *meta) { delete meta; }));
    EXPECT_CALL(*mock, media_meta_data_set_cstring(meta_data, _, _))
            .Times(1);
    EXPECT_CALL(*mock, media_meta_data_set_int32(meta_data, _, _))
            .Times(6);

    EXPECT_CALL(*mock, media_meta_data_get_key_id(_))
            .Times(0);
    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_MIME))
            .Times(1);
    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_COLOR_FORMAT))
            .Times(1);
    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_WIDTH))
            .Times(1);
    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_HEIGHT))
            .Times(1);
    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_STRIDE))
            .Times(1);
    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_SLICE_HEIGHT))
            .Times(1);
    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_FRAMERATE))
            .Times(1);

    auto source = new DummyMediaSourceWrapper;

    EXPECT_CALL(*mock, media_source_create())
            .Times(1)
            .WillRepeatedly(Return(source));
    EXPECT_CALL(*mock, media_source_release(source))
            .Times(1)
            .WillOnce(Invoke([](MediaSourceWrapper *source) { delete source; }));
    EXPECT_CALL(*mock, media_source_set_format(source, meta_data))
            .Times(1);
    EXPECT_CALL(*mock, media_source_set_start_callback(source, _, _))
            .Times(1);
    EXPECT_CALL(*mock, media_source_set_stop_callback(source, _, _))
            .Times(1);
    EXPECT_CALL(*mock, media_source_set_pause_callback(source, _, _))
            .Times(1);
    EXPECT_CALL(*mock, media_source_set_read_callback(source, _, NotNull()))
            .Times(1);

    auto codec_source = new DummyMediaCodecSource;

    EXPECT_CALL(*mock, media_codec_source_create(NotNull(), NotNull(), _))
            .Times(1)
            .WillRepeatedly(Return(codec_source));
    EXPECT_CALL(*mock, media_codec_source_release(codec_source))
            .Times(1)
            .WillOnce(Invoke([](MediaCodecSourceWrapper *source) { delete source; }));

    EXPECT_TRUE(encoder->Configure(config));

    // We can configure the encoder only once
    EXPECT_FALSE(encoder->Configure(config));

    const auto stored_config = encoder->Configuration();
    EXPECT_EQ(config, stored_config);
}

TEST_F(H264EncoderFixture, CorrectStartAndStopBehavior) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    ExpectValidConfiguration(config, mock);
    ExpectValidStartAndStop(mock);

    EXPECT_FALSE(encoder->Start());
    EXPECT_FALSE(encoder->Stop());

    EXPECT_TRUE(encoder->Configure(config));

    EXPECT_TRUE(encoder->Start());
    EXPECT_TRUE(encoder->Running());
    EXPECT_FALSE(encoder->Start());
    EXPECT_TRUE(encoder->Stop());
    EXPECT_FALSE(encoder->Running());
    EXPECT_FALSE(encoder->Stop());
}

TEST_F(H264EncoderFixture, StartFailsCorrectly) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    ExpectValidConfiguration(config, mock);

    EXPECT_TRUE(encoder->Configure(config));

    EXPECT_CALL(*mock, media_codec_source_start(_))
            .Times(1)
            .WillRepeatedly(Return(false));

    EXPECT_FALSE(encoder->Start());
}

TEST_F(H264EncoderFixture, StopFailsCorrectly) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    ExpectValidConfiguration(config, mock);

    EXPECT_TRUE(encoder->Configure(config));

    EXPECT_CALL(*mock, media_codec_source_start(_))
            .Times(1)
            .WillRepeatedly(Return(true));
    EXPECT_CALL(*mock, media_codec_source_stop(_))
            // Will be called twice as the d'tor also calls Stop to
            // ensure the encoder is stopped correctly on cleanup.
            .Times(2)
            .WillRepeatedly(Return(false));

    EXPECT_CALL(*mock_report, Started())
            .Times(1);

    EXPECT_TRUE(encoder->Start());
    EXPECT_FALSE(encoder->Stop());
}

TEST_F(H264EncoderFixture, RequestIDRFrame) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    ExpectValidConfiguration(config, mock);

    encoder->SendIDRFrame();

    EXPECT_TRUE(encoder->Configure(config));

    EXPECT_CALL(*mock, media_codec_source_request_idr_frame(_))
            .Times(1);

    encoder->SendIDRFrame();
}

TEST_F(H264EncoderFixture, ReturnsPackedBufferAndReleaseProperly) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    ExpectValidConfiguration(config, mock);

    EXPECT_TRUE(encoder->Configure(config));
    EXPECT_NE(nullptr, source_read_callback);
    EXPECT_NE(nullptr, source_read_callback_data);

    ExpectValidStartAndStop(mock);

    EXPECT_TRUE(encoder->Start());

    auto anwb = new ANativeWindowBuffer;
    anwb->handle = new native_handle_t;

    auto buffer_delegate = std::make_shared<MockBufferDelegate>();

    auto input_buffer = mcs::video::Buffer::Create(anwb);
    input_buffer->SetDelegate(buffer_delegate);
    auto now = mcs::Utils::GetNowUs();
    input_buffer->SetTimestamp(now);

    EXPECT_CALL(*mock_report, ReceivedInputBuffer(_))
            .Times(1);

    encoder->QueueBuffer(input_buffer);

    MediaBufferWrapper *output_buffer = nullptr;
    auto mbuf = new DummyMediaBufferWrapper;
    size_t mbuf_size = sizeof(buffer_handle_t) + 4;
    auto mbuf_data = new uint8_t[mbuf_size];

    EXPECT_CALL(*mock, media_buffer_create(mbuf_size))
            .Times(1)
            .WillRepeatedly(Return(mbuf));

    EXPECT_CALL(*mock, media_buffer_get_data(mbuf))
            .Times(1)
            .WillRepeatedly(Return(mbuf_data));

    EXPECT_CALL(*mock, media_buffer_ref(mbuf))
            .Times(1);

    EXPECT_CALL(*mock, media_buffer_get_meta_data(mbuf))
            .Times(1);
    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_TIME))
            .Times(1)
            .WillRepeatedly(Return(42));
    EXPECT_CALL(*mock, media_meta_data_set_int64(_, 42, now));

    MediaBufferReturnCallback return_callback;
    void *return_callback_data = nullptr;

    EXPECT_CALL(*mock, media_buffer_set_return_callback(mbuf, NotNull(), NotNull()))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<1>(&return_callback),
                                  SaveArg<2>(&return_callback_data)));


    EXPECT_CALL(*mock, media_buffer_set_return_callback(mbuf, nullptr, nullptr))
            .Times(1);

    EXPECT_CALL(*mock_report, BeganFrame(_))
            .Times(1);

    EXPECT_EQ(0, source_read_callback(&output_buffer, source_read_callback_data));

    EXPECT_EQ(mbuf, output_buffer);

    EXPECT_CALL(*buffer_delegate, OnBufferFinished(input_buffer))
            .Times(1);

    EXPECT_CALL(*mock, media_buffer_release(output_buffer))
            .Times(1);

    return_callback(output_buffer, return_callback_data);

    // Doesn't crash or fail badly when no real buffer is returned
    return_callback(nullptr, return_callback_data);

    EXPECT_TRUE(encoder->Stop());

    delete anwb;
    delete anwb->handle;
}

TEST_F(H264EncoderFixture, SourceReadFailsForInvalidState) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    ExpectValidConfiguration(config, mock);

    ExpectValidStartAndStop(mock);

    EXPECT_TRUE(encoder->Configure(config));
    EXPECT_NE(nullptr, source_read_callback);
    EXPECT_NE(nullptr, source_read_callback_data);

    // Must fail as the encoder isn't started yet
    EXPECT_GE(0, source_read_callback(reinterpret_cast<MediaBufferWrapper**>(1), source_read_callback_data));

    // Must also fail when encoder isn't started and when started
    EXPECT_GE(0, source_read_callback(nullptr, source_read_callback_data));
    EXPECT_TRUE(encoder->Start());
    EXPECT_GE(0, source_read_callback(nullptr, source_read_callback_data));
}

TEST_F(H264EncoderFixture, QueueBufferDoesNotCrashWhenInactive) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    auto buffer = mcs::video::Buffer::Create(nullptr);
    encoder->QueueBuffer(buffer);
}

TEST_F(H264EncoderFixture, ExecuteFailForInvalidState) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    auto config = encoder->DefaultConfiguration();

    EXPECT_FALSE(encoder->Execute());
}

TEST_F(H264EncoderFixture, ExecuteFailsForFailedSourceRead) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    ExpectValidConfiguration(config, mock);
    ExpectValidStartAndStop(mock);

    EXPECT_TRUE(encoder->Configure(config));

    EXPECT_TRUE(encoder->Start());

    EXPECT_CALL(*mock, media_codec_source_read(_, _))
            .Times(1)
            .WillRepeatedly(Return(false));

    EXPECT_FALSE(encoder->Execute());

    EXPECT_TRUE(encoder->Stop());
}

TEST_F(H264EncoderFixture, ExecuteProvidesBuffers) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder_delegate = std::make_shared<MockEncoderDelegate>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    ExpectValidConfiguration(config, mock);
    ExpectValidStartAndStop(mock);

    encoder->SetDelegate(encoder_delegate);

    EXPECT_TRUE(encoder->Configure(config));
    EXPECT_TRUE(encoder->Start());

    auto input_buffer = new DummyMediaBufferWrapper;
    mcs::TimestampUs input_buffer_timestamp = 23ll;

    EXPECT_CALL(*mock, media_codec_source_read(_, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<1>(input_buffer), Return(true)));

    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_TIME))
            .Times(1)
            .WillRepeatedly(Return(1));
    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_IS_CODEC_CONFIG))
            .Times(1)
            .WillRepeatedly(Return(2));

    auto meta_data = new DummyMediaMetaDataWrapper;

    EXPECT_CALL(*mock, media_buffer_get_meta_data(input_buffer))
            .Times(2)
            .WillRepeatedly(Return(meta_data));

    EXPECT_CALL(*mock, media_meta_data_find_int64(meta_data, 1, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<2>(input_buffer_timestamp), Return(true)));
    EXPECT_CALL(*mock, media_meta_data_find_int32(meta_data, 2, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<2>(0), Return(true)));

    EXPECT_CALL(*mock, media_buffer_get_refcount(input_buffer))
            .Times(1)
            .WillRepeatedly(Return(0));

    EXPECT_CALL(*mock, media_buffer_destroy(input_buffer))
            .Times(1);

    EXPECT_CALL(*encoder_delegate, OnBufferAvailable(_))
            .Times(1);
    EXPECT_CALL(*encoder_delegate, OnBufferWithCodecConfig(_))
            .Times(0);

    EXPECT_CALL(*mock_report, FinishedFrame(_))
            .Times(1);

    EXPECT_TRUE(encoder->Execute());

    EXPECT_TRUE(encoder->Stop());
}

TEST_F(H264EncoderFixture, HandsBuffersWithCodecSpecificDataBack) {
    auto mock = std::make_shared<mcs::test::android::MockMedia>();

    auto encoder_delegate = std::make_shared<MockEncoderDelegate>();

    auto encoder = mcs::android::H264Encoder::Create(mock_report);

    const auto config = encoder->DefaultConfiguration();

    ExpectValidConfiguration(config, mock);
    ExpectValidStartAndStop(mock);

    encoder->SetDelegate(encoder_delegate);

    EXPECT_TRUE(encoder->Configure(config));
    EXPECT_TRUE(encoder->Start());

    auto input_buffer = new DummyMediaBufferWrapper;
    mcs::TimestampUs input_buffer_timestamp = 23ll;

    EXPECT_CALL(*mock, media_codec_source_read(_, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<1>(input_buffer), Return(true)));

    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_TIME))
            .Times(1)
            .WillRepeatedly(Return(1));
    EXPECT_CALL(*mock, media_meta_data_get_key_id(MEDIA_META_DATA_KEY_IS_CODEC_CONFIG))
            .Times(1)
            .WillRepeatedly(Return(2));

    auto meta_data = new DummyMediaMetaDataWrapper;

    EXPECT_CALL(*mock, media_buffer_get_meta_data(input_buffer))
            .Times(2)
            .WillRepeatedly(Return(meta_data));

    EXPECT_CALL(*mock, media_meta_data_find_int64(meta_data, 1, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<2>(input_buffer_timestamp), Return(true)));
    EXPECT_CALL(*mock, media_meta_data_find_int32(meta_data, 2, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<2>(1 /* marks this as a buffer with CSD */), Return(true)));

    EXPECT_CALL(*mock, media_buffer_get_refcount(input_buffer))
            .Times(1)
            .WillRepeatedly(Return(0));

    EXPECT_CALL(*mock, media_buffer_destroy(input_buffer))
            .Times(1);

    EXPECT_CALL(*encoder_delegate, OnBufferWithCodecConfig(_))
            .Times(1);
    EXPECT_CALL(*encoder_delegate, OnBufferAvailable(_))
            .Times(1);

    EXPECT_CALL(*mock_report, FinishedFrame(_))
            .Times(1);

    EXPECT_TRUE(encoder->Execute());

    EXPECT_TRUE(encoder->Stop());
}
