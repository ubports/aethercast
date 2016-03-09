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

#include "mockmedia.h"

namespace {
mcs::test::android::MockMedia *global_mock = nullptr;
}

namespace mcs {
namespace test {
namespace android {

MockMedia::MockMedia() {
    using namespace testing;
    assert(global_mock == NULL && "Only one mock object per process is allowed");

    global_mock = this;
}

MockMedia::~MockMedia() {
    global_mock = nullptr;
}

} // namespace android
} // namespace tests
} // namespace mcss


MediaSourceWrapper* media_source_create() {
    return global_mock->media_source_create();
}

void media_source_release(MediaSourceWrapper *source) {
    global_mock->media_source_release(source);
}

void media_source_set_format(MediaSourceWrapper *source, MediaMetaDataWrapper *meta) {
    global_mock->media_source_set_format(source, meta);
}

void media_source_set_start_callback(MediaSourceWrapper *source, MediaSourceStartCallback callback,
                                          void *user_data) {
    global_mock->media_source_set_start_callback(source, callback, user_data);
}

void media_source_set_stop_callback(MediaSourceWrapper *source, MediaSourceStopCallback callback,
                                         void *user_data) {
    global_mock->media_source_set_stop_callback(source, callback, user_data);
}

void media_source_set_read_callback(MediaSourceWrapper *source, MediaSourceReadCallback callback,
                                         void *user_data) {
    global_mock->media_source_set_read_callback(source, callback, user_data);
}

void media_source_set_pause_callback(MediaSourceWrapper *source, MediaSourcePauseCallback callback,
                                          void *user_data) {
    global_mock->media_source_set_pause_callback(source, callback, user_data);
}

MediaCodecSourceWrapper* media_codec_source_create(MediaMessageWrapper *format, MediaSourceWrapper *source, int flags) {
    return global_mock->media_codec_source_create(format, source, flags);
}

void media_codec_source_release(MediaCodecSourceWrapper *source) {
    return global_mock->media_codec_source_release(source);
}

MediaNativeWindowHandle* media_codec_source_get_native_window_handle(MediaCodecSourceWrapper *source) {
    return global_mock->media_codec_source_get_native_window_handle(source);
}

bool media_codec_source_start(MediaCodecSourceWrapper *source) {
    return global_mock->media_codec_source_start(source);
}

bool media_codec_source_stop(MediaCodecSourceWrapper *source) {
    return global_mock->media_codec_source_stop(source);
}

bool media_codec_source_pause(MediaCodecSourceWrapper *source) {
    return global_mock->media_codec_source_pause(source);
}

bool media_codec_source_read(MediaCodecSourceWrapper *source, MediaBufferWrapper **buffer) {
    return global_mock->media_codec_source_read(source, buffer);
}

bool media_codec_source_request_idr_frame(MediaCodecSourceWrapper *source) {
    return global_mock->media_codec_source_request_idr_frame(source);
}

MediaMessageWrapper* media_message_create() {
    return global_mock->media_message_create();
}

void media_message_release(MediaMessageWrapper *msg) {
    global_mock->media_message_release(msg);
}

void media_message_set_int32(MediaMessageWrapper *msg, const char *name, int32_t value) {
    global_mock->media_message_set_int32(msg, name, value);
}

void media_message_set_string(MediaMessageWrapper *msg, const char *name, const char *value, ssize_t len) {
    global_mock->media_message_set_string(msg, name, value, len);
}

uint32_t media_meta_data_get_key_id(int key) {
    return global_mock->media_meta_data_get_key_id(key);
}

MediaMetaDataWrapper *media_meta_data_create() {
    return global_mock->media_meta_data_create();
}

void media_meta_data_release(MediaMetaDataWrapper *meta_data) {
    return global_mock->media_meta_data_release(meta_data);
}

bool media_meta_data_set_cstring(MediaMetaDataWrapper *meta_data, uint32_t key, const char *value) {
    return global_mock->media_meta_data_set_cstring(meta_data, key, value);
}

bool media_meta_data_set_int32(MediaMetaDataWrapper *meta_data, uint32_t key, int32_t value) {
    return global_mock->media_meta_data_set_int32(meta_data, key, value);
}

bool media_meta_data_set_int64(MediaMetaDataWrapper *meta_data, uint32_t key, int64_t value) {
    return global_mock->media_meta_data_set_int64(meta_data, key, value);
}

bool media_meta_data_find_int32(MediaMetaDataWrapper *meta_data, uint32_t key, int32_t *value) {
    return global_mock->media_meta_data_find_int32(meta_data, key, value);
}

bool media_meta_data_find_int64(MediaMetaDataWrapper *meta_data, uint32_t key, int64_t *value) {
    return global_mock->media_meta_data_find_int64(meta_data, key, value);
}

MediaBufferWrapper* media_buffer_create(size_t size) {
    return global_mock->media_buffer_create(size);
}

void media_buffer_destroy(MediaBufferWrapper *buffer) {
    global_mock->media_buffer_destroy(buffer);
}

void media_buffer_release(MediaBufferWrapper *buffer) {
    global_mock->media_buffer_release(buffer);
}

void media_buffer_ref(MediaBufferWrapper *buffer) {
    global_mock->media_buffer_ref(buffer);
}

int media_buffer_get_refcount(MediaBufferWrapper *buffer) {
    return global_mock->media_buffer_get_refcount(buffer);
}

void* media_buffer_get_data(MediaBufferWrapper *buffer) {
    return global_mock->media_buffer_get_data(buffer);
}

size_t media_buffer_get_size(MediaBufferWrapper *buffer) {
    return global_mock->media_buffer_get_size(buffer);
}

MediaMetaDataWrapper* media_buffer_get_meta_data(MediaBufferWrapper *buffer) {
    return global_mock->media_buffer_get_meta_data(buffer);
}

void media_buffer_set_return_callback(MediaBufferWrapper *buffer, MediaBufferReturnCallback callback, void *user_data) {
    global_mock->media_buffer_set_return_callback(buffer, callback, user_data);
}
