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

#include <chrono>
#include <thread>

#include <boost/concept_check.hpp>

#include "ac/logger.h"

#include "ac/mir/screencast.h"
#include "ac/mir/streamrenderer.h"

namespace {
static constexpr const char *kStreamRendererThreadName{"StreamRenderer"};
static constexpr unsigned int kNumBufferSlots{2};
}

namespace ac {
namespace mir {

StreamRenderer::StreamRenderer(const video::BufferProducer::Ptr &buffer_producer,
                               const video::BaseEncoder::Ptr &encoder,
                               const video::RendererReport::Ptr &report) :
    report_(report),
    buffer_producer_(buffer_producer),
    encoder_(encoder),
    width_(buffer_producer->OutputMode().width),
    height_(buffer_producer->OutputMode().height),
    input_buffers_(ac::video::BufferQueue::Create(BufferSlots())),
    target_iteration_time_((1. / encoder_->Configuration().framerate) * std::micro::den) {
}

StreamRenderer::~StreamRenderer() {
    Stop();
}

bool StreamRenderer::Execute() {
    ac::TimestampUs iteration_start_time = ac::Utils::GetNowUs();

    // Wait until we have free slots again and all buffers we produced
    // went through the pipeline.
    if (!input_buffers_->WaitForSlots())
        return true;

    report_->BeganFrame();

    // This will trigger the rendering/compositing process inside mir
    // and will block until that is done and we received a new buffer
    buffer_producer_->SwapBuffers();

    const auto native_buffer = buffer_producer_->CurrentBuffer();

    auto buffer = ac::video::Buffer::Create(native_buffer);
    buffer->SetDelegate(shared_from_this());

    // FIXME: at optimum we would get the timestamp directly supplied
    // from our producer but as long as that isn't available we don't
    // have any other chance and need to do it here.
    buffer->SetTimestamp(ac::Utils::GetNowUs());

    input_buffers_->Push(buffer);

    encoder_->QueueBuffer(buffer);

    report_->FinishedFrame(buffer->Timestamp());

    // Calculate how long we have to wait until we can render the next
    // frame to keep our framerate constant.
    ac::TimestampUs iteration_time = ac::Utils::GetNowUs() - iteration_start_time;
    ac::TimestampUs sleep_time = target_iteration_time_ - iteration_time;
    if (sleep_time > 0)
        std::this_thread::sleep_for(std::chrono::microseconds(sleep_time));

    return true;
}

void StreamRenderer::OnBufferFinished(const video::Buffer::Ptr &buffer) {
    boost::ignore_unused_variable_warning(buffer);

    // We're currently relying on the buffers to come back in order so
    // we can safely remove the head from the queue here which then
    // gives us a free slot at the beginning which will be filled by
    // the renderer again.
    input_buffers_->Pop();
}

bool StreamRenderer::Start() {
    DEBUG("Everything successfully setup; Starting recording now %dx%d@%d",
              width_, height_, encoder_->Configuration().framerate);

    return true;
}

bool StreamRenderer::Stop() {
    return true;
}

std::string StreamRenderer::Name() const {
    return kStreamRendererThreadName;
}

std::uint32_t StreamRenderer::BufferSlots() const {
    return kNumBufferSlots;
}

} // namespace mir
} // namespace ac
