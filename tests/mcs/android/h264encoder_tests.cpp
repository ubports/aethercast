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

#include <system/window.h>

#include "mcs/android/h264encoder.h"

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

class MockMediaAPI : public mcs::android::MediaAPI {
public:
    typedef std::shared_ptr<MockMediaAPI> Ptr;

    MOCK_METHOD0(MediaSource_Create, MediaSourceWrapper*(void));
    MOCK_METHOD1(MediaSource_Release, void(MediaSourceWrapper*));
    MOCK_METHOD2(MediaSource_SetFormat, void(MediaSourceWrapper*, MediaMetaDataWrapper *meta));
    MOCK_METHOD3(MediaSource_SetStartCallback, void(MediaSourceWrapper*, MediaSourceStartCallback, void*));
    MOCK_METHOD3(MediaSource_SetStopCallback, void(MediaSourceWrapper*, MediaSourceStopCallback, void*));
    MOCK_METHOD3(MediaSource_SetReadCallback, void(MediaSourceWrapper*, MediaSourceReadCallback, void*));
    MOCK_METHOD3(MediaSource_SetPauseCallback, void(MediaSourceWrapper*, MediaSourcePauseCallback, void*));

    MOCK_METHOD3(MediaCodecSource_Create, MediaCodecSourceWrapper*(MediaMessageWrapper*, MediaSourceWrapper*, int));
    MOCK_METHOD1(MediaCodecSource_Release, void(MediaCodecSourceWrapper*));
    MOCK_METHOD1(MediaCodecSource_GetNativeWindowHandle, MediaNativeWindowHandle*(MediaCodecSourceWrapper*));
    MOCK_METHOD1(MediaCodecSource_Start, bool(MediaCodecSourceWrapper*));
    MOCK_METHOD1(MediaCodecSource_Stop, bool(MediaCodecSourceWrapper*));
    MOCK_METHOD1(MediaCodecSource_Pause, bool(MediaCodecSourceWrapper*));
    MOCK_METHOD2(MediaCodecSource_Read, bool(MediaCodecSourceWrapper*, MediaBufferWrapper**));
    MOCK_METHOD1(MediaCodecSource_RequestIDRFrame, bool(MediaCodecSourceWrapper*));

    MOCK_METHOD0(MediaMessage_Create, MediaMessageWrapper*());
    MOCK_METHOD1(MediaMessage_Release, void(MediaMessageWrapper*));
    MOCK_METHOD3(MediaMessage_SetInt32, void(MediaMessageWrapper*, const char*, int32_t));
    MOCK_METHOD4(MediaMessage_SetString, void(MediaMessageWrapper*, const char*, const char*, ssize_t));

    MOCK_METHOD0(MediaMetaData_Create, MediaMetaDataWrapper*());
    MOCK_METHOD1(MediaMetaData_Release, void(MediaMetaDataWrapper*));
    MOCK_METHOD3(MediaMetaData_SetCString, bool(MediaMetaDataWrapper*, uint32_t, const char*));
    MOCK_METHOD3(MediaMetaData_SetInt32, bool(MediaMetaDataWrapper*, uint32_t, int32_t));
    MOCK_METHOD3(MediaMetaData_SetInt64, bool(MediaMetaDataWrapper*, uint32_t, int64_t));
    MOCK_METHOD3(MediaMetaData_FindInt32, bool(MediaMetaDataWrapper*, uint32_t, int32_t*));
    MOCK_METHOD3(MediaMetaData_FindInt64, bool(MediaMetaDataWrapper*, uint32_t, int64_t*));
    MOCK_METHOD1(MediaMetaData_GetKeyId, uint32_t(int));

    MOCK_METHOD1(MediaBuffer_Create, MediaBufferWrapper*(size_t));
    MOCK_METHOD1(MediaBuffer_Destroy, void(MediaBufferWrapper*));
    MOCK_METHOD1(MediaBuffer_Release, void(MediaBufferWrapper*));
    MOCK_METHOD1(MediaBuffer_Ref, void(MediaBufferWrapper*));
    MOCK_METHOD1(MediaBuffer_GetRefCount, int(MediaBufferWrapper*));
    MOCK_METHOD1(MediaBuffer_GetData, void*(MediaBufferWrapper*));
    MOCK_METHOD1(MediaBuffer_GetSize, size_t(MediaBufferWrapper*));
    MOCK_METHOD1(MediaBuffer_GetMetaData, MediaMetaDataWrapper*(MediaBufferWrapper*));
    MOCK_METHOD3(MediaBuffer_SetReturnCallback, void(MediaBufferWrapper*, MediaBufferReturnCallback, void*));
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
    void ExpectValidConfiguration(const mcs::video::BaseEncoder::Config &config, const MockMediaAPI::Ptr &api) {
        auto message = new DummyMediaMessageWrapper;

        EXPECT_CALL(*api, MediaMessage_Create())
                .Times(1)
                .WillRepeatedly(Return(message));
        EXPECT_CALL(*api, MediaMessage_Release(message))
                .Times(1)
                .WillOnce(Invoke([](MediaMessageWrapper *msg) { delete msg; }));
        EXPECT_CALL(*api, MediaMessage_SetString(message, _, _, _))
                .Times(AtLeast(0));
        EXPECT_CALL(*api, MediaMessage_SetInt32(message, _, _))
                .Times(AtLeast(0));

        auto source = new DummyMediaSourceWrapper;

        EXPECT_CALL(*api, MediaSource_Create())
                .Times(1)
                .WillRepeatedly(Return(source));
        EXPECT_CALL(*api, MediaSource_Release(source))
                .Times(1)
                .WillOnce(Invoke([](MediaSourceWrapper *source) { delete source; }));

        auto meta_data = new DummyMediaMetaDataWrapper;

        EXPECT_CALL(*api, MediaMetaData_Create())
                .Times(1)
                .WillRepeatedly(Return(meta_data));
        EXPECT_CALL(*api, MediaMetaData_Release(meta_data))
                .Times(1)
                .WillOnce(Invoke([](MediaMetaDataWrapper *meta) { delete meta; }));
        EXPECT_CALL(*api, MediaMetaData_SetCString(meta_data, _, _))
                .Times(AtLeast(0));
        EXPECT_CALL(*api, MediaMetaData_SetInt32(meta_data, _, _))
                .Times(AtLeast(0));

        EXPECT_CALL(*api, MediaMetaData_GetKeyId(_))
                .Times(AtLeast(0))
                .WillRepeatedly(Return(0));

        EXPECT_CALL(*api, MediaSource_SetFormat(source, meta_data))
                .Times(1);
        EXPECT_CALL(*api, MediaSource_SetStartCallback(source, _, _))
                .Times(1);
        EXPECT_CALL(*api, MediaSource_SetStopCallback(source, _, _))
                .Times(1);
        EXPECT_CALL(*api, MediaSource_SetPauseCallback(source, _, _))
                .Times(1);
        EXPECT_CALL(*api, MediaSource_SetReadCallback(source, _, _))
                .Times(1)
                .WillRepeatedly(DoAll(SaveArg<1>(&source_read_callback),
                                      SaveArg<2>(&source_read_callback_data)));

        auto codec_source = new DummyMediaCodecSource;

        EXPECT_CALL(*api, MediaCodecSource_Create(message, source, _))
                .Times(1)
                .WillRepeatedly(Return(codec_source));
        EXPECT_CALL(*api, MediaCodecSource_Release(codec_source))
                .Times(1)
                .WillOnce(Invoke([](MediaCodecSourceWrapper *source) { delete source; }));
    }

    void ExpectValidStartAndStop(const MockMediaAPI::Ptr &api) {
        EXPECT_CALL(*api, MediaCodecSource_Start(_))
                .Times(1)
                .WillRepeatedly(Return(true));

        EXPECT_CALL(*api, MediaCodecSource_Stop(_))
                .Times(1)
                .WillRepeatedly(Return(true));
    }

    MediaSourceReadCallback source_read_callback;
    void *source_read_callback_data;
};
}

TEST(H264Encoder, ValidDefaultConfiguration) {
    auto config = mcs::android::H264Encoder::DefaultConfiguration();
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

TEST(H264Encoder, MediaMessageCreationFails) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    EXPECT_CALL(*api, MediaMessage_Create())
            .Times(1)
            .WillRepeatedly(Return(nullptr));

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_FALSE(encoder->Configure(config));
}

TEST(H264Encoder, MediaSourceCreationFails) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    auto message = new DummyMediaMessageWrapper;

    EXPECT_CALL(*api, MediaMessage_Create())
            .Times(1)
            .WillRepeatedly(Return(message));
    EXPECT_CALL(*api, MediaMessage_Release(message))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetString(message, _, _, _))
            .Times(AtLeast(0));
    EXPECT_CALL(*api, MediaMessage_SetInt32(message, _, _))
            .Times(AtLeast(0));

    EXPECT_CALL(*api, MediaSource_Create())
            .Times(1)
            .WillRepeatedly(Return(nullptr));

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_FALSE(encoder->Configure(config));
}

TEST(H264Encoder, MediaMetaDataCreationFails) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    auto message = new DummyMediaMessageWrapper;

    EXPECT_CALL(*api, MediaMessage_Create())
            .Times(1)
            .WillRepeatedly(Return(message));
    EXPECT_CALL(*api, MediaMessage_Release(message))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetString(message, _, _, _))
            .Times(AtLeast(0));
    EXPECT_CALL(*api, MediaMessage_SetInt32(message, _, _))
            .Times(AtLeast(0));

    auto source = new DummyMediaSourceWrapper;

    EXPECT_CALL(*api, MediaSource_Create())
            .Times(1)
            .WillRepeatedly(Return(source));
    EXPECT_CALL(*api, MediaSource_Release(source))
            .Times(1);

    EXPECT_CALL(*api, MediaMetaData_Create())
            .Times(1)
            .WillRepeatedly(Return(nullptr));

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_FALSE(encoder->Configure(config));
}

TEST(H264Encoder, MediaCodecSourceCreationFails) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    auto message = new DummyMediaMessageWrapper;

    EXPECT_CALL(*api, MediaMessage_Create())
            .Times(1)
            .WillRepeatedly(Return(message));
    EXPECT_CALL(*api, MediaMessage_Release(message))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetString(message, _, _, _))
            .Times(AtLeast(0));
    EXPECT_CALL(*api, MediaMessage_SetInt32(message, _, _))
            .Times(AtLeast(0));

    auto source = new DummyMediaSourceWrapper;

    EXPECT_CALL(*api, MediaSource_Create())
            .Times(1)
            .WillRepeatedly(Return(source));
    EXPECT_CALL(*api, MediaSource_Release(source))
            .Times(1);

    auto meta_data = new DummyMediaMetaDataWrapper;

    EXPECT_CALL(*api, MediaMetaData_Create())
            .Times(1)
            .WillRepeatedly(Return(meta_data));
    EXPECT_CALL(*api, MediaMetaData_Release(meta_data))
            .Times(1);
    EXPECT_CALL(*api, MediaMetaData_SetCString(meta_data, _, _))
            .Times(AtLeast(0));
    EXPECT_CALL(*api, MediaMetaData_SetInt32(meta_data, _, _))
            .Times(AtLeast(0));

    EXPECT_CALL(*api, MediaMetaData_GetKeyId(_))
            .Times(AtLeast(0))
            .WillRepeatedly(Return(0));

    EXPECT_CALL(*api, MediaSource_SetFormat(source, meta_data))
            .Times(1);
    EXPECT_CALL(*api, MediaSource_SetStartCallback(source, _, _))
            .Times(1);
    EXPECT_CALL(*api, MediaSource_SetStopCallback(source, _, _))
            .Times(1);
    EXPECT_CALL(*api, MediaSource_SetPauseCallback(source, _, _))
            .Times(1);
    EXPECT_CALL(*api, MediaSource_SetReadCallback(source, _, _))
            .Times(1);

    EXPECT_CALL(*api, MediaCodecSource_Create(message, source, _))
            .Times(1)
            .WillRepeatedly(Return(nullptr));

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_FALSE(encoder->Configure(config));
}

TEST(H264Encoder, CorrectConfiguration) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();
    config.width = 1280;
    config.height = 720;
    config.framerate = 30;
    config.profile_idc = 1;
    config.level_idc = 2;
    config.constraint_set = 3;

    auto format_message = new DummyMediaMessageWrapper;

    EXPECT_CALL(*api, MediaMessage_Create())
            .Times(1)
            .WillRepeatedly(Return(format_message));
    EXPECT_CALL(*api, MediaMessage_Release(format_message))
            .Times(1)
            .WillOnce(Invoke([](MediaMessageWrapper *msg) { delete msg; }));

    EXPECT_CALL(*api, MediaMessage_SetString(format_message, StrEq("mime"), StrEq("video/avc"), 0))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("store-metadata-in-buffers"), 1))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("store-metadata-in-buffers-output"), 0))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("width"), config.width))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("height"), config.height))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("stride"), config.width))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("slice-height"), config.width))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("color-format"), kOMXColorFormatAndroidOpaque))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("bitrate"), config.bitrate))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("bitrate-mode"), kOMXVideoControlRateConstant))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("frame-rate"), config.framerate))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("intra-refresh-mode"), config.intra_refresh_mode))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("intra-refresh-CIR-mbs"), 360))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("i-frame-interval"), config.i_frame_interval))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("prepend-sps-pps-to-idr-frames"), 1))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("profile-idc"), 1))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("level-idc"), 2))
            .Times(1);
    EXPECT_CALL(*api, MediaMessage_SetInt32(format_message, StrEq("constraint-set"), 3))
            .Times(1);

    auto meta_data = new DummyMediaMetaDataWrapper;

    EXPECT_CALL(*api, MediaMetaData_Create())
            .Times(1)
            .WillRepeatedly(Return(meta_data));
    EXPECT_CALL(*api, MediaMetaData_Release(meta_data))
            .Times(1)
            .WillRepeatedly(Invoke([](MediaMetaDataWrapper *meta) { delete meta; }));
    EXPECT_CALL(*api, MediaMetaData_SetCString(meta_data, _, _))
            .Times(1);
    EXPECT_CALL(*api, MediaMetaData_SetInt32(meta_data, _, _))
            .Times(6);

    EXPECT_CALL(*api, MediaMetaData_GetKeyId(_))
            .Times(0);
    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_MIME))
            .Times(1);
    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_COLOR_FORMAT))
            .Times(1);
    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_WIDTH))
            .Times(1);
    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_HEIGHT))
            .Times(1);
    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_STRIDE))
            .Times(1);
    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_SLICE_HEIGHT))
            .Times(1);
    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_FRAMERATE))
            .Times(1);

    auto source = new DummyMediaSourceWrapper;

    EXPECT_CALL(*api, MediaSource_Create())
            .Times(1)
            .WillRepeatedly(Return(source));
    EXPECT_CALL(*api, MediaSource_Release(source))
            .Times(1)
            .WillOnce(Invoke([](MediaSourceWrapper *source) { delete source; }));
    EXPECT_CALL(*api, MediaSource_SetFormat(source, meta_data))
            .Times(1);
    EXPECT_CALL(*api, MediaSource_SetStartCallback(source, _, _))
            .Times(1);
    EXPECT_CALL(*api, MediaSource_SetStopCallback(source, _, _))
            .Times(1);
    EXPECT_CALL(*api, MediaSource_SetPauseCallback(source, _, _))
            .Times(1);
    EXPECT_CALL(*api, MediaSource_SetReadCallback(source, _, NotNull()))
            .Times(1);

    auto codec_source = new DummyMediaCodecSource;

    EXPECT_CALL(*api, MediaCodecSource_Create(NotNull(), NotNull(), _))
            .Times(1)
            .WillRepeatedly(Return(codec_source));
    EXPECT_CALL(*api, MediaCodecSource_Release(codec_source))
            .Times(1)
            .WillOnce(Invoke([](MediaCodecSourceWrapper *source) { delete source; }));

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_TRUE(encoder->Configure(config));

    // We can configure the encoder only once
    EXPECT_FALSE(encoder->Configure(config));

    auto stored_config = encoder->Configuration();
    EXPECT_EQ(config, stored_config);
}

TEST_F(H264EncoderFixture, CorrectStartAndStopBehavior) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    ExpectValidConfiguration(config, api);

    auto encoder = mcs::android::H264Encoder::Create(api);

    ExpectValidStartAndStop(api);

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
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    ExpectValidConfiguration(config, api);

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_TRUE(encoder->Configure(config));

    EXPECT_CALL(*api, MediaCodecSource_Start(_))
            .Times(1)
            .WillRepeatedly(Return(false));

    EXPECT_FALSE(encoder->Start());
}

TEST_F(H264EncoderFixture, StopFailsCorrectly) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    ExpectValidConfiguration(config, api);

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_TRUE(encoder->Configure(config));

    EXPECT_CALL(*api, MediaCodecSource_Start(_))
            .Times(1)
            .WillRepeatedly(Return(true));
    EXPECT_CALL(*api, MediaCodecSource_Stop(_))
            // Will be called twice as the d'tor also calls Stop to
            // ensure the encoder is stopped correctly on cleanup.
            .Times(2)
            .WillRepeatedly(Return(false));

    EXPECT_TRUE(encoder->Start());
    EXPECT_FALSE(encoder->Stop());
}

TEST_F(H264EncoderFixture, ReturnsCorrectNativeWindowHandle) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    ExpectValidConfiguration(config, api);

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_EQ(nullptr, encoder->NativeWindowHandle());

    EXPECT_TRUE(encoder->Configure(config));

    auto expected_handle = reinterpret_cast<void*>(1);

    EXPECT_CALL(*api, MediaCodecSource_GetNativeWindowHandle(_))
            .Times(1)
            .WillRepeatedly(Return(expected_handle));

    EXPECT_EQ(expected_handle, encoder->NativeWindowHandle());
}

TEST_F(H264EncoderFixture, RequestIDRFrame) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    ExpectValidConfiguration(config, api);

    auto encoder = mcs::android::H264Encoder::Create(api);

    encoder->SendIDRFrame();

    EXPECT_TRUE(encoder->Configure(config));

    EXPECT_CALL(*api, MediaCodecSource_RequestIDRFrame(_))
            .Times(1);

    encoder->SendIDRFrame();
}

TEST_F(H264EncoderFixture, ReturnsPackedBufferAndReleaseProperly) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    ExpectValidConfiguration(config, api);

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_TRUE(encoder->Configure(config));
    EXPECT_NE(nullptr, source_read_callback);
    EXPECT_NE(nullptr, source_read_callback_data);

    ExpectValidStartAndStop(api);

    EXPECT_TRUE(encoder->Start());

    auto anwb = new ANativeWindowBuffer;
    anwb->handle = new native_handle_t;

    auto buffer_delegate = std::make_shared<MockBufferDelegate>();

    auto input_buffer = mcs::video::Buffer::Create(anwb);
    input_buffer->SetDelegate(buffer_delegate);
    auto now = mcs::Utils::GetNowUs();
    input_buffer->SetTimestamp(now);

    encoder->QueueBuffer(input_buffer);

    MediaBufferWrapper *output_buffer = nullptr;
    auto mbuf = new DummyMediaBufferWrapper;
    size_t mbuf_size = sizeof(buffer_handle_t) + 4;
    auto mbuf_data = new uint8_t[mbuf_size];

    EXPECT_CALL(*api, MediaBuffer_Create(mbuf_size))
            .Times(1)
            .WillRepeatedly(Return(mbuf));

    EXPECT_CALL(*api, MediaBuffer_GetData(mbuf))
            .Times(1)
            .WillRepeatedly(Return(mbuf_data));

    EXPECT_CALL(*api, MediaBuffer_Ref(mbuf))
            .Times(1);

    EXPECT_CALL(*api, MediaBuffer_GetMetaData(mbuf))
            .Times(1);
    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_TIME))
            .Times(1)
            .WillRepeatedly(Return(42));
    EXPECT_CALL(*api, MediaMetaData_SetInt64(_, 42, now));

    MediaBufferReturnCallback return_callback;
    void *return_callback_data = nullptr;

    EXPECT_CALL(*api, MediaBuffer_SetReturnCallback(mbuf, NotNull(), NotNull()))
            .Times(1)
            .WillRepeatedly(DoAll(SaveArg<1>(&return_callback),
                                  SaveArg<2>(&return_callback_data)));


    EXPECT_CALL(*api, MediaBuffer_SetReturnCallback(mbuf, nullptr, nullptr))
            .Times(1);

    EXPECT_EQ(0, source_read_callback(&output_buffer, source_read_callback_data));

    EXPECT_EQ(mbuf, output_buffer);

    EXPECT_CALL(*buffer_delegate, OnBufferFinished(input_buffer))
            .Times(1);

    EXPECT_CALL(*api, MediaBuffer_Release(output_buffer))
            .Times(1);

    return_callback(output_buffer, return_callback_data);

    // Doesn't crash or fail badly when no real buffer is returned
    return_callback(nullptr, return_callback_data);

    EXPECT_TRUE(encoder->Stop());

    delete anwb;
    delete anwb->handle;
}

TEST_F(H264EncoderFixture, SourceReadFailsForInvalidState) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    ExpectValidConfiguration(config, api);

    auto encoder = mcs::android::H264Encoder::Create(api);

    ExpectValidStartAndStop(api);

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
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    auto encoder = mcs::android::H264Encoder::Create(api);

    auto buffer = mcs::video::Buffer::Create(nullptr);
    encoder->QueueBuffer(buffer);
}

TEST_F(H264EncoderFixture, ExecuteFailForInvalidState) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_FALSE(encoder->Execute());
}

TEST_F(H264EncoderFixture, ExecuteFailsForFailedSourceRead) {
    auto api = std::make_shared<MockMediaAPI>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    ExpectValidConfiguration(config, api);
    ExpectValidStartAndStop(api);

    auto encoder = mcs::android::H264Encoder::Create(api);

    EXPECT_TRUE(encoder->Configure(config));

    EXPECT_TRUE(encoder->Start());

    EXPECT_CALL(*api, MediaCodecSource_Read(_, _))
            .Times(1)
            .WillRepeatedly(Return(false));

    EXPECT_FALSE(encoder->Execute());

    EXPECT_TRUE(encoder->Stop());
}

TEST_F(H264EncoderFixture, ExecuteProvidesBuffers) {
    auto api = std::make_shared<MockMediaAPI>();

    auto encoder_delegate = std::make_shared<MockEncoderDelegate>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    ExpectValidConfiguration(config, api);
    ExpectValidStartAndStop(api);

    auto encoder = mcs::android::H264Encoder::Create(api);
    encoder->SetDelegate(encoder_delegate);

    EXPECT_TRUE(encoder->Configure(config));
    EXPECT_TRUE(encoder->Start());

    auto input_buffer = new DummyMediaBufferWrapper;
    mcs::TimestampUs input_buffer_timestamp = 23ll;

    EXPECT_CALL(*api, MediaCodecSource_Read(_, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<1>(input_buffer), Return(true)));

    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_TIME))
            .Times(1)
            .WillRepeatedly(Return(1));
    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_IS_CODEC_CONFIG))
            .Times(1)
            .WillRepeatedly(Return(2));

    auto meta_data = new DummyMediaMetaDataWrapper;

    EXPECT_CALL(*api, MediaBuffer_GetMetaData(input_buffer))
            .Times(2)
            .WillRepeatedly(Return(meta_data));

    EXPECT_CALL(*api, MediaMetaData_FindInt64(meta_data, 1, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<2>(input_buffer_timestamp), Return(true)));
    EXPECT_CALL(*api, MediaMetaData_FindInt32(meta_data, 2, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<2>(0), Return(true)));

    EXPECT_CALL(*api, MediaBuffer_GetRefCount(input_buffer))
            .Times(1)
            .WillRepeatedly(Return(0));

    EXPECT_CALL(*api, MediaBuffer_Destroy(input_buffer))
            .Times(1);

    EXPECT_CALL(*encoder_delegate, OnBufferAvailable(_))
            .Times(1);
    EXPECT_CALL(*encoder_delegate, OnBufferWithCodecConfig(_))
            .Times(0);

    EXPECT_TRUE(encoder->Execute());

    EXPECT_TRUE(encoder->Stop());
}

TEST_F(H264EncoderFixture, HandsBuffersWithCodecSpecificDataBack) {
    auto api = std::make_shared<MockMediaAPI>();

    auto encoder_delegate = std::make_shared<MockEncoderDelegate>();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();

    ExpectValidConfiguration(config, api);
    ExpectValidStartAndStop(api);

    auto encoder = mcs::android::H264Encoder::Create(api);
    encoder->SetDelegate(encoder_delegate);

    EXPECT_TRUE(encoder->Configure(config));
    EXPECT_TRUE(encoder->Start());

    auto input_buffer = new DummyMediaBufferWrapper;
    mcs::TimestampUs input_buffer_timestamp = 23ll;

    EXPECT_CALL(*api, MediaCodecSource_Read(_, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<1>(input_buffer), Return(true)));

    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_TIME))
            .Times(1)
            .WillRepeatedly(Return(1));
    EXPECT_CALL(*api, MediaMetaData_GetKeyId(MEDIA_META_DATA_KEY_IS_CODEC_CONFIG))
            .Times(1)
            .WillRepeatedly(Return(2));

    auto meta_data = new DummyMediaMetaDataWrapper;

    EXPECT_CALL(*api, MediaBuffer_GetMetaData(input_buffer))
            .Times(2)
            .WillRepeatedly(Return(meta_data));

    EXPECT_CALL(*api, MediaMetaData_FindInt64(meta_data, 1, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<2>(input_buffer_timestamp), Return(true)));
    EXPECT_CALL(*api, MediaMetaData_FindInt32(meta_data, 2, _))
            .Times(1)
            .WillRepeatedly(DoAll(SetArgPointee<2>(1 /* marks this as a buffer with CSD */), Return(true)));

    EXPECT_CALL(*api, MediaBuffer_GetRefCount(input_buffer))
            .Times(1)
            .WillRepeatedly(Return(0));

    EXPECT_CALL(*api, MediaBuffer_Destroy(input_buffer))
            .Times(1);

    EXPECT_CALL(*encoder_delegate, OnBufferWithCodecConfig(_))
            .Times(1);
    EXPECT_CALL(*encoder_delegate, OnBufferAvailable(_))
            .Times(1);

    EXPECT_TRUE(encoder->Execute());

    EXPECT_TRUE(encoder->Stop());
}
