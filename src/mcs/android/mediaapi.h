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

#ifndef MCS_ANDROID_MEDIAAPI_H_
#define MCS_ANDROID_MEDIAAPI_H_

#include <hybris/media/media_codec_source_layer.h>

#include <memory>

#include "mcs/non_copyable.h"

namespace mcs {
namespace android {

class MediaAPI : public mcs::NonCopyable {
public:
    typedef std::shared_ptr<MediaAPI> Ptr;

    static Ptr CreateDefault();

    virtual MediaSourceWrapper* MediaSource_Create() = 0;
    virtual void MediaSource_Release(MediaSourceWrapper *source) = 0;
    virtual void MediaSource_SetFormat(MediaSourceWrapper *source, MediaMetaDataWrapper *meta) = 0;
    virtual void MediaSource_SetStartCallback(MediaSourceWrapper *source, MediaSourceStartCallback callback,
                                              void *user_data) = 0;
    virtual void MediaSource_SetStopCallback(MediaSourceWrapper *source, MediaSourceStopCallback callback,
                                             void *user_data) = 0;
    virtual void MediaSource_SetReadCallback(MediaSourceWrapper *source, MediaSourceReadCallback callback,
                                             void *user_data) = 0;
    virtual void MediaSource_SetPauseCallback(MediaSourceWrapper *source, MediaSourcePauseCallback callback,
                                              void *user_data) = 0;

    virtual MediaCodecSourceWrapper* MediaCodecSource_Create(MediaMessageWrapper *format, MediaSourceWrapper *source, int flags) = 0;
    virtual void MediaCodecSource_Release(MediaCodecSourceWrapper *source) = 0;
    virtual MediaMetaDataWrapper* MediaCodecSource_GetFormat(MediaCodecSourceWrapper *source) = 0;
    virtual MediaNativeWindowHandle* MediaCodecSource_GetNativeWindowHandle(MediaCodecSourceWrapper *source) = 0;
    virtual bool MediaCodecSource_Start(MediaCodecSourceWrapper *source) = 0;
    virtual bool MediaCodecSource_Stop(MediaCodecSourceWrapper *source) = 0;
    virtual bool MediaCodecSource_Pause(MediaCodecSourceWrapper *source) = 0;
    virtual bool MediaCodecSource_Read(MediaCodecSourceWrapper *source, MediaBufferWrapper **buffer) = 0;
    virtual bool MediaCodecSource_RequestIDRFrame(MediaCodecSourceWrapper *source) = 0;

    virtual MediaMessageWrapper* MediaMessage_Create() = 0;
    virtual void MediaMessage_Release(MediaMessageWrapper *msg) = 0;
    virtual void MediaMessage_SetInt32(MediaMessageWrapper *msg, const char *name, int32_t value) = 0;
    virtual void MediaMessage_SetString(MediaMessageWrapper *msg, const char *name, const char *value, ssize_t len) = 0;

    virtual MediaMetaDataWrapper *MediaMetaData_Create() = 0;
    virtual void MediaMetaData_Release(MediaMetaDataWrapper *meta_data) = 0;
    virtual bool MediaMetaData_SetCString(MediaMetaDataWrapper *meta_data, uint32_t key, const char *value) = 0;
    virtual bool MediaMetaData_SetInt32(MediaMetaDataWrapper *meta_data, uint32_t key, int32_t value) = 0;
    virtual bool MediaMetaData_SetInt64(MediaMetaDataWrapper *meta_data, uint32_t key, int64_t value) = 0;
    virtual bool MediaMetaData_FindInt32(MediaMetaDataWrapper *meta_data, uint32_t key, int32_t *value) = 0;
    virtual bool MediaMetaData_FindInt64(MediaMetaDataWrapper *meta_data, uint32_t key, int64_t *value) = 0;
    virtual uint32_t MediaMetaData_GetKeyId(int key) = 0;

    virtual MediaBufferWrapper* MediaBuffer_Create(size_t size) = 0;
    virtual void MediaBuffer_Destroy(MediaBufferWrapper *buffer) = 0;
    virtual void MediaBuffer_Release(MediaBufferWrapper *buffer) = 0;
    virtual void MediaBuffer_Ref(MediaBufferWrapper *buffer) = 0;
    virtual int MediaBuffer_GetRefCount(MediaBufferWrapper *buffer) = 0;
    virtual void* MediaBuffer_GetData(MediaBufferWrapper *buffer) = 0;
    virtual size_t MediaBuffer_GetSize(MediaBufferWrapper *buffer) = 0;
    virtual MediaMetaDataWrapper* MediaBuffer_GetMetaData(MediaBufferWrapper *buffer) = 0;
    virtual void MediaBuffer_SetReturnCallback(MediaBufferWrapper *buffer, MediaBufferReturnCallback callback, void *user_data) = 0;
};

} // namespace android
} // namespace mcs

#endif
