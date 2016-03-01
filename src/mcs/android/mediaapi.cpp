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

#include "mcs/android/mediaapi.h"

namespace mcs {
namespace android {

class MediaAPIImpl : public MediaAPI {
public:
    MediaSourceWrapper* MediaSource_Create() {
        return media_source_create();
    }

    void MediaSource_Release(MediaSourceWrapper *source) override {
        media_source_release(source);
    }

    void MediaSource_SetFormat(MediaSourceWrapper *source, MediaMetaDataWrapper *meta) override {
        media_source_set_format(source, meta);
    }

    void MediaSource_SetStartCallback(MediaSourceWrapper *source, MediaSourceStartCallback callback,
                                              void *user_data) override {
        media_source_set_start_callback(source, callback, user_data);
    }

    void MediaSource_SetStopCallback(MediaSourceWrapper *source, MediaSourceStopCallback callback,
                                             void *user_data) override {
        media_source_set_stop_callback(source, callback, user_data);
    }

    void MediaSource_SetReadCallback(MediaSourceWrapper *source, MediaSourceReadCallback callback,
                                             void *user_data) override {
        media_source_set_read_callback(source, callback, user_data);
    }

    void MediaSource_SetPauseCallback(MediaSourceWrapper *source, MediaSourcePauseCallback callback,
                                              void *user_data) override {
        media_source_set_pause_callback(source, callback, user_data);
    }

    MediaCodecSourceWrapper* MediaCodecSource_Create(MediaMessageWrapper *format, MediaSourceWrapper *source, int flags) {
        return media_codec_source_create(format, source, flags);
    }

    void MediaCodecSource_Release(MediaCodecSourceWrapper *source) {
        return media_codec_source_release(source);
    }

    MediaNativeWindowHandle* MediaCodecSource_GetNativeWindowHandle(MediaCodecSourceWrapper *source) override {
        return media_codec_source_get_native_window_handle(source);
    }

    bool MediaCodecSource_Start(MediaCodecSourceWrapper *source) override {
        return media_codec_source_start(source);
    }

    bool MediaCodecSource_Stop(MediaCodecSourceWrapper *source) override {
        return media_codec_source_stop(source);
    }

    bool MediaCodecSource_Pause(MediaCodecSourceWrapper *source) override {
        return media_codec_source_pause(source);
    }

    bool MediaCodecSource_Read(MediaCodecSourceWrapper *source, MediaBufferWrapper **buffer) override {
        return media_codec_source_read(source, buffer);
    }

    bool MediaCodecSource_RequestIDRFrame(MediaCodecSourceWrapper *source) override {
        return media_codec_source_request_idr_frame(source);
    }

    MediaMessageWrapper* MediaMessage_Create() override {
        return media_message_create();
    }

    void MediaMessage_Release(MediaMessageWrapper *msg) override {
        media_message_release(msg);
    }

    void MediaMessage_SetInt32(MediaMessageWrapper *msg, const char *name, int32_t value) override {
        media_message_set_int32(msg, name, value);
    }

    void MediaMessage_SetString(MediaMessageWrapper *msg, const char *name, const char *value, ssize_t len) override {
        media_message_set_string(msg, name, value, len);
    }

    uint32_t MediaMetaData_GetKeyId(int key) override {
        return media_meta_data_get_key_id(key);
    }

    MediaMetaDataWrapper *MediaMetaData_Create() override {
        return media_meta_data_create();
    }

    void MediaMetaData_Release(MediaMetaDataWrapper *meta_data) override {
        return media_meta_data_release(meta_data);
    }

    bool MediaMetaData_SetCString(MediaMetaDataWrapper *meta_data, uint32_t key, const char *value) override {
        return media_meta_data_set_cstring(meta_data, key, value);
    }

    bool MediaMetaData_SetInt32(MediaMetaDataWrapper *meta_data, uint32_t key, int32_t value) override {
        return media_meta_data_set_int32(meta_data, key, value);
    }

    bool MediaMetaData_SetInt64(MediaMetaDataWrapper *meta_data, uint32_t key, int64_t value) override {
        return media_meta_data_set_int64(meta_data, key, value);
    }

    bool MediaMetaData_FindInt32(MediaMetaDataWrapper *meta_data, uint32_t key, int32_t *value) override {
        return media_meta_data_find_int32(meta_data, key, value);
    }

    bool MediaMetaData_FindInt64(MediaMetaDataWrapper *meta_data, uint32_t key, int64_t *value) override {
        return media_meta_data_find_int64(meta_data, key, value);
    }

    MediaBufferWrapper* MediaBuffer_Create(size_t size) override {
        return media_buffer_create(size);
    }

    void MediaBuffer_Destroy(MediaBufferWrapper *buffer) override {
        media_buffer_destroy(buffer);
    }

    void MediaBuffer_Release(MediaBufferWrapper *buffer) override {
        media_buffer_release(buffer);
    }

    void MediaBuffer_Ref(MediaBufferWrapper *buffer) override {
        media_buffer_ref(buffer);
    }

    int MediaBuffer_GetRefCount(MediaBufferWrapper *buffer) {
        return media_buffer_get_refcount(buffer);
    }

    void* MediaBuffer_GetData(MediaBufferWrapper *buffer) {
        return media_buffer_get_data(buffer);
    }

    size_t MediaBuffer_GetSize(MediaBufferWrapper *buffer) {
        return media_buffer_get_size(buffer);
    }

    MediaMetaDataWrapper* MediaBuffer_GetMetaData(MediaBufferWrapper *buffer) {
        return media_buffer_get_meta_data(buffer);
    }

    void MediaBuffer_SetReturnCallback(MediaBufferWrapper *buffer, MediaBufferReturnCallback callback, void *user_data) {
        media_buffer_set_return_callback(buffer, callback, user_data);
    }
};

MediaAPI::Ptr MediaAPI::CreateDefault() {
    return std::shared_ptr<MediaAPI>(new MediaAPIImpl);
}

} // namespace android
} // namespace mcs
