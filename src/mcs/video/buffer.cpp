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

#include <memory.h>

#include "mcs/video/buffer.h"

namespace mcs {
namespace video {

Buffer::Ptr Buffer::Create(uint32_t capacity, mcs::TimestampUs timestamp) {
    auto buffer = std::shared_ptr<Buffer>(new Buffer(timestamp));
    buffer->Allocate(capacity);
    return buffer;
}

Buffer::Ptr Buffer::Create(uint8_t *data, uint32_t length) {
    auto buffer = std::shared_ptr<Buffer>(new Buffer);
    buffer->Allocate(length);
    ::memcpy(buffer->data_, data, length);
    return buffer;
}

Buffer::Ptr Buffer::Create(void *native_handle) {
    auto buffer = std::shared_ptr<Buffer>(new Buffer);
    buffer->native_handle_ = native_handle;
    return buffer;
}

Buffer::Buffer() :
    capacity_(0),
    length_(0),
    offset_(0),
    data_(nullptr),
    timestamp_(0),
    native_handle_(nullptr) {
}

Buffer::Buffer(int64_t timestamp) :
    capacity_(0),
    length_(0),
    offset_(0),
    data_(nullptr),
    timestamp_(timestamp),
    native_handle_(nullptr) {
}

Buffer::~Buffer() {
    if (data_)
        free(data_);
}

void Buffer::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

void Buffer::Release() {
    if (auto sp = delegate_.lock())
        sp->OnBufferFinished(shared_from_this());
}

void Buffer::SetRange(uint32_t offset, uint32_t length) {
    if (length > capacity_ || offset < 0 || offset > capacity_)
        return;

    offset_ = offset;
    length_ = length;
}

void Buffer::SetTimestamp(int64_t timestamp) {
    timestamp_ = timestamp;
}

void Buffer::Allocate(uint32_t capacity) {
    if (data_)
        return;

    data_ = new uint8_t[capacity];
    ::memset(data_, 0, sizeof(data_));
    capacity_ = capacity;
    length_ = capacity;
    offset_ = 0;
}

} // namespace video
} // namespace mcs
