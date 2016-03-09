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

#ifndef MCS_VIDEO_BUFFERQUEUE_H_
#define MCS_VIDEO_BUFFERQUEUE_H_

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "mcs/video/buffer.h"

namespace mcs {
namespace video {

class BufferQueue {
public:
    typedef std::shared_ptr<BufferQueue> Ptr;

    static Ptr Create(uint32_t max_size = 0);

    ~BufferQueue();

    mcs::video::Buffer::Ptr Next();
    mcs::video::Buffer::Ptr Front();

    void Lock();
    void Unlock();

    void Push(const mcs::video::Buffer::Ptr &buffer);
    void PushUnlocked(const mcs::video::Buffer::Ptr &buffer);

    mcs::video::Buffer::Ptr Pop();
    mcs::video::Buffer::Ptr PopUnlocked();

    bool WaitForSlots(const std::chrono::milliseconds &timeout = std::chrono::milliseconds{1});
    bool WaitToBeFilled(const std::chrono::milliseconds &timeout = std::chrono::milliseconds{1});

    bool IsLimited() const { return max_size_ != 0; }
    bool IsFull();
    bool IsEmpty();

    int Size();

private:
    BufferQueue(uint32_t max_size);

    bool WaitFor(const std::function<bool()> &pred, const std::chrono::milliseconds &timeout);

private:
    uint32_t max_size_;
    std::queue<mcs::video::Buffer::Ptr> queue_;
    std::mutex mutex_;
    std::condition_variable lock_;
};

} // namespace video
} // namespace mcs

#endif
