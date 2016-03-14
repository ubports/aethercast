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

#ifndef MCS_MIR_STREAMRENDERER_H_
#define MCS_MIR_STREAMRENDERER_H_

#include <memory>
#include <thread>
#include <mutex>
#include <queue>

#include "mcs/common/executable.h"

#include "mcs/mir/streamrenderer.h"
#include "mcs/mir/screencast.h"

#include "mcs/video/baseencoder.h"
#include "mcs/video/bufferqueue.h"
#include "mcs/video/rendererreport.h"

namespace mcs {
namespace mir {
class StreamRenderer : public std::enable_shared_from_this<StreamRenderer>,
                       public mcs::common::Executable,
                       public mcs::video::Buffer::Delegate {
public:
    static constexpr unsigned int kNumTextures{2};

    typedef std::shared_ptr<StreamRenderer> Ptr;

    StreamRenderer(const Screencast::Ptr &connector, const video::BaseEncoder::Ptr &encoder,
                   const video::RendererReport::Ptr  &report);
    ~StreamRenderer();

    void SetDimensions(unsigned int width, unsigned int height);

    // From mcs::video::Buffer::Delegate
    void OnBufferFinished(const mcs::video::Buffer::Ptr &buffer);

    // From mcs::common::Executable
    bool Start() override;
    bool Stop() override;
    bool Execute() override;
    std::string Name() const override;

private:
    video::RendererReport::Ptr report_;
    Screencast::Ptr connector_;
    video::BaseEncoder::Ptr encoder_;
    unsigned int width_;
    unsigned int height_;
    mcs::video::BufferQueue::Ptr input_buffers_;
};
} // namespace mir
} // namespace mcs

#endif
