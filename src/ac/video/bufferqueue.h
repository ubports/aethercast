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

#ifndef AC_VIDEO_BUFFERQUEUE_H_
#define AC_VIDEO_BUFFERQUEUE_H_

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "ac/video/buffer.h"

namespace ac {
namespace video {

class BufferQueue {
public:
    typedef std::shared_ptr<BufferQueue> Ptr;

    static Ptr Create(uint32_t max_size = 0);

    ~BufferQueue();

    ac::video::Buffer::Ptr Next();
    ac::video::Buffer::Ptr Front();

    void Lock();
    void Unlock();

    void Push(const ac::video::Buffer::Ptr &buffer);
    void PushUnlocked(const ac::video::Buffer::Ptr &buffer);

    ac::video::Buffer::Ptr Pop();
    ac::video::Buffer::Ptr PopUnlocked();

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
    std::queue<ac::video::Buffer::Ptr> queue_;
    std::mutex mutex_;
    std::condition_variable lock_;
};

} // namespace video
} // namespace ac

#endif
