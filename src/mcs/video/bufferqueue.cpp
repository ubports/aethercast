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

#include "mcs/video/bufferqueue.h"

namespace mcs {
namespace video {

BufferQueue::Ptr BufferQueue::Create(uint32_t max_size) {
    return std::shared_ptr<BufferQueue>(new BufferQueue(max_size));
}

BufferQueue::BufferQueue(uint32_t max_size) :
    max_size_(max_size) {
}

BufferQueue::~BufferQueue() {
}

void BufferQueue::Lock() {
    mutex_.lock();
}

void BufferQueue::PushUnlocked(const mcs::video::Buffer::Ptr &buffer) {
    queue_.push(buffer);
}

void BufferQueue::Unlock() {
    mutex_.unlock();
    lock_.notify_one();
}

mcs::video::Buffer::Ptr BufferQueue::Front() {
    std::unique_lock<std::mutex> l(mutex_);
    return queue_.front();
}

mcs::video::Buffer::Ptr BufferQueue::Next() {
    // We will block here forever until we get a new buffer
    WaitToBeFilled(std::chrono::milliseconds{-1});
    std::unique_lock<std::mutex> l(mutex_);
    auto buffer = queue_.front();
    queue_.pop();
    return buffer;
}

void BufferQueue::Push(const mcs::video::Buffer::Ptr &buffer) {
    std::unique_lock<std::mutex> l(mutex_);
    if (IsLimited() && queue_.size() >= max_size_)
        return;
    queue_.push(buffer);
    lock_.notify_one();
}

mcs::video::Buffer::Ptr BufferQueue::Pop() {
    std::unique_lock<std::mutex> l(mutex_);
    auto buffer = queue_.front();
    queue_.pop();
    lock_.notify_one();
    return buffer;
}

mcs::video::Buffer::Ptr BufferQueue::PopUnlocked() {
    if (queue_.size() == 0)
        return nullptr;

    auto buffer = queue_.front();
    queue_.pop();
    return buffer;
}

bool BufferQueue::WaitFor(const std::function<bool()> &pred, const std::chrono::milliseconds &timeout) {
    std::unique_lock<std::mutex> l(mutex_);

    if (timeout.count() >= 0) {
        auto now = std::chrono::system_clock::now();
        return lock_.wait_until(l, now + timeout, pred);
    }

    lock_.wait(l, pred);
    return true;
}

bool BufferQueue::WaitToBeFilled(const std::chrono::milliseconds &timeout) {
    if (IsFull())
        return true;

    return WaitFor([&]() { return !queue_.empty(); }, timeout);
}

bool BufferQueue::WaitForSlots(const std::chrono::milliseconds &timeout) {
    if (!IsLimited())
        return true;

    return WaitFor([&]() { return queue_.size() < max_size_; }, timeout);
}

bool BufferQueue::IsFull() {
    if (!IsLimited())
        return false;

    std::unique_lock<std::mutex> l(mutex_);
    return queue_.size() == max_size_;
}

bool BufferQueue::IsEmpty() {
    std::unique_lock<std::mutex> l(mutex_);
    return queue_.size() == 0;
}

int BufferQueue::Size() {
    std::unique_lock<std::mutex> l(mutex_);
    return queue_.size();
}

} // namespace video
} // namespace mcs
