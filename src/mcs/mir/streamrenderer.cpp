/*
 * Copyright (C) 2015 Canonical, Ltd.
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

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <system/window.h>

#include <chrono>
#include <thread>

#include <boost/concept_check.hpp>

#include "mcs/logger.h"

#include "mcs/video/statistics.h"

#include "mcs/mir/screencast.h"
#include "mcs/mir/streamrenderer.h"

namespace {
static constexpr const char *kStreamRendererThreadName{"StreamRenderer"};
static constexpr unsigned int kNumBufferSlots{2};
}

namespace mcs {
namespace mir {

StreamRenderer::Ptr StreamRenderer::Create(const Screencast::Ptr &connector, const video::BaseEncoder::Ptr &encoder,
                                           const video::RendererReport::Ptr &report) {
    return std::shared_ptr<StreamRenderer>(new StreamRenderer(connector, encoder, report));
}

StreamRenderer::StreamRenderer(const Screencast::Ptr &connector, const video::BaseEncoder::Ptr &encoder,
                               const video::RendererReport::Ptr &report) :
    report_(report),
    connector_(connector),
    encoder_(encoder),
    running_(false),
    width_(0),
    height_(0),
    input_buffers_(mcs::video::BufferQueue::Create(kNumBufferSlots)) {
}

StreamRenderer::~StreamRenderer() {
    Stop();
}

void StreamRenderer::RenderThread() {
    mcs::Utils::SetThreadName(kStreamRendererThreadName);

    MCS_DEBUG("Everything successfully setup; Starting recording now %dx%d@%d",
              width_, height_, encoder_->Configuration().framerate);

    static const mcs::TimestampUs target_iteration_time = (1. / encoder_->Configuration().framerate) * 1000000ll;

    while (running_) {
        mcs::TimestampUs iteration_start_time = mcs::Utils::GetNowUs();

        report_->BeganFrame();

        if (!input_buffers_->WaitForSlots())
            continue;

        // This will trigger the rendering/compositing process inside mir
        // and will block until that is done and we received a new buffer
        connector_->SwapBuffersSync();

        auto native_buffer = connector_->CurrentBuffer();

        auto buffer = mcs::video::Buffer::Create(native_buffer);
        buffer->SetDelegate(shared_from_this());

        // FIXME: at optimum we would get the timestamp directly supplied
        // from mir but as long as that isn't available we don't have any
        // other chance and need to do it here.
        buffer->SetTimestamp(mcs::Utils::GetNowUs());

        input_buffers_->Push(buffer);

        encoder_->QueueBuffer(buffer);

        report_->FinishedFrame(buffer->Timestamp());

        // Calculate how long we have to wait until we can render the next
        // frame to keep our framerate constant.
        mcs::TimestampUs iteration_time = mcs::Utils::GetNowUs() - iteration_start_time;
        int64_t sleep_time = target_iteration_time - iteration_time;
        if (sleep_time > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_time));
    }
}

void StreamRenderer::OnBufferFinished(const video::Buffer::Ptr &buffer) {
    boost::ignore_unused_variable_warning(buffer);

    // We're currently relying on the buffers to come back in order so
    // we can safely remove the head from the queue here which then
    // gives us a free slot at the beginning which will be filled by
    // the renderer again.
    input_buffers_->Pop();
}

void StreamRenderer::SetDimensions(unsigned int width, unsigned int height) {
    width_ = width;
    height_ = height;
}

void StreamRenderer::StartThreaded() {
    if (running_)
        return;

    auto output_mode = connector_->OutputMode();

    if (width_ == 0 || height_ == 0) {
        width_ = output_mode.width;
        height_ = output_mode.height;
    }

    running_ = true;

    render_thread_ = std::thread(&StreamRenderer::RenderThread, this);
}

void StreamRenderer::Stop() {
    if (!running_)
        return;

    running_ = false;
    render_thread_.join();
}

} // namespace mir
} // namespace mcs
