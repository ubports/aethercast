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

#ifndef AC_MIR_STREAMRENDERER_H_
#define AC_MIR_STREAMRENDERER_H_

#include <memory>
#include <thread>
#include <mutex>
#include <queue>

#include "ac/common/executable.h"

#include "ac/mir/streamrenderer.h"
#include "ac/mir/screencast.h"

#include "ac/video/baseencoder.h"
#include "ac/video/bufferqueue.h"
#include "ac/video/bufferproducer.h"
#include "ac/video/rendererreport.h"

namespace ac {
namespace mir {
class StreamRenderer : public std::enable_shared_from_this<StreamRenderer>,
                       public ac::common::Executable,
                       public ac::video::Buffer::Delegate {
public:
    static constexpr unsigned int kNumTextures{2};

    typedef std::shared_ptr<StreamRenderer> Ptr;

    StreamRenderer(const video::BufferProducer::Ptr &buffer_producer,
                   const video::BaseEncoder::Ptr &encoder,
                   const video::RendererReport::Ptr  &report);
    ~StreamRenderer();

    std::uint32_t BufferSlots() const;

    // From ac::video::Buffer::Delegate
    void OnBufferFinished(const ac::video::Buffer::Ptr &buffer);

    // From ac::common::Executable
    bool Start() override;
    bool Stop() override;
    bool Execute() override;
    std::string Name() const override;

private:
    video::RendererReport::Ptr report_;
    video::BufferProducer::Ptr buffer_producer_;
    video::BaseEncoder::Ptr encoder_;
    unsigned int width_;
    unsigned int height_;
    ac::video::BufferQueue::Ptr input_buffers_;
    ac::TimestampUs target_iteration_time_;
};
} // namespace mir
} // namespace ac

#endif
