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

#ifndef MCS_TEST_ANDROID_MOCKMEDIA_H_
#define MCS_TEST_ANDROID_MOCKMEDIA_H_

#include <memory>

#include <gmock/gmock.h>

#include <hybris/media/media_codec_source_layer.h>

namespace mcs {
namespace test {
namespace android {

class MockMedia {
public:
    typedef std::shared_ptr<MockMedia> Ptr;

    MockMedia();
    ~MockMedia();

    MOCK_METHOD0(media_source_create, MediaSourceWrapper*(void));
    MOCK_METHOD1(media_source_release, void(MediaSourceWrapper*));
    MOCK_METHOD2(media_source_set_format, void(MediaSourceWrapper*, MediaMetaDataWrapper *meta));
    MOCK_METHOD3(media_source_set_start_callback, void(MediaSourceWrapper*, MediaSourceStartCallback, void*));
    MOCK_METHOD3(media_source_set_stop_callback, void(MediaSourceWrapper*, MediaSourceStopCallback, void*));
    MOCK_METHOD3(media_source_set_read_callback, void(MediaSourceWrapper*, MediaSourceReadCallback, void*));
    MOCK_METHOD3(media_source_set_pause_callback, void(MediaSourceWrapper*, MediaSourcePauseCallback, void*));

    MOCK_METHOD3(media_codec_source_create, MediaCodecSourceWrapper*(MediaMessageWrapper*, MediaSourceWrapper*, int));
    MOCK_METHOD1(media_codec_source_release, void(MediaCodecSourceWrapper*));
    MOCK_METHOD1(media_codec_source_get_native_window_handle, MediaNativeWindowHandle*(MediaCodecSourceWrapper*));
    MOCK_METHOD1(media_codec_source_start, bool(MediaCodecSourceWrapper*));
    MOCK_METHOD1(media_codec_source_stop, bool(MediaCodecSourceWrapper*));
    MOCK_METHOD1(media_codec_source_pause, bool(MediaCodecSourceWrapper*));
    MOCK_METHOD2(media_codec_source_read, bool(MediaCodecSourceWrapper*, MediaBufferWrapper**));
    MOCK_METHOD1(media_codec_source_request_idr_frame, bool(MediaCodecSourceWrapper*));

    MOCK_METHOD0(media_message_create, MediaMessageWrapper*());
    MOCK_METHOD1(media_message_release, void(MediaMessageWrapper*));
    MOCK_METHOD3(media_message_set_int32, void(MediaMessageWrapper*, const char*, int32_t));
    MOCK_METHOD4(media_message_set_string, void(MediaMessageWrapper*, const char*, const char*, ssize_t));

    MOCK_METHOD0(media_meta_data_create, MediaMetaDataWrapper*());
    MOCK_METHOD1(media_meta_data_release, void(MediaMetaDataWrapper*));
    MOCK_METHOD3(media_meta_data_set_cstring, bool(MediaMetaDataWrapper*, uint32_t, const char*));
    MOCK_METHOD3(media_meta_data_set_int32, bool(MediaMetaDataWrapper*, uint32_t, int32_t));
    MOCK_METHOD3(media_meta_data_set_int64, bool(MediaMetaDataWrapper*, uint32_t, int64_t));
    MOCK_METHOD3(media_meta_data_find_int32, bool(MediaMetaDataWrapper*, uint32_t, int32_t*));
    MOCK_METHOD3(media_meta_data_find_int64, bool(MediaMetaDataWrapper*, uint32_t, int64_t*));
    MOCK_METHOD1(media_meta_data_get_key_id, uint32_t(int));

    MOCK_METHOD1(media_buffer_create, MediaBufferWrapper*(size_t));
    MOCK_METHOD1(media_buffer_destroy, void(MediaBufferWrapper*));
    MOCK_METHOD1(media_buffer_release, void(MediaBufferWrapper*));
    MOCK_METHOD1(media_buffer_ref, void(MediaBufferWrapper*));
    MOCK_METHOD1(media_buffer_get_refcount, int(MediaBufferWrapper*));
    MOCK_METHOD1(media_buffer_get_data, void*(MediaBufferWrapper*));
    MOCK_METHOD1(media_buffer_get_size, size_t(MediaBufferWrapper*));
    MOCK_METHOD1(media_buffer_get_meta_data, MediaMetaDataWrapper*(MediaBufferWrapper*));
    MOCK_METHOD3(media_buffer_set_return_callback, void(MediaBufferWrapper*, MediaBufferReturnCallback, void*));
};

} // namespace android
} // namespace tests
} // namespace mcs

#endif
