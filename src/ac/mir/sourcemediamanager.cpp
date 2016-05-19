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

#include "ac/logger.h"
#include "ac/keep_alive.h"

#include "ac/common/threadedexecutor.h"
#include "ac/common/threadedexecutorfactory.h"

#include "ac/network/udpstream.h"

#include "ac/report/reportfactory.h"

#include "ac/video/videoformat.h"
#include "ac/video/displayoutput.h"

#include "ac/streaming/mpegtspacketizer.h"
#include "ac/streaming/rtpsender.h"

#include "ac/mir/sourcemediamanager.h"

#include "ac/android/h264encoder.h"

namespace {
// Number of milliseconds was choosen by measurement
static constexpr std::chrono::milliseconds kStreamDelayOnPlay{300};
}

namespace ac {
namespace mir {

SourceMediaManager::SourceMediaManager(const std::string &remote_address,
                                       const ac::common::ExecutorFactory::Ptr &executor_factory,
                                       const ac::video::BufferProducer::Ptr &producer,
                                       const ac::video::BaseEncoder::Ptr &encoder,
                                       const ac::network::Stream::Ptr &output_stream,
                                       const ac::report::ReportFactory::Ptr &report_factory) :
    state_(State::Stopped),
    remote_address_(remote_address),
    producer_(producer),
    encoder_(encoder),
    output_stream_(output_stream),
    report_factory_(report_factory),
    pipeline_(executor_factory, 4),
    delay_timeout_(0) {
}

SourceMediaManager::~SourceMediaManager() {
    if (state_ != State::Stopped)
        pipeline_.Stop();
}

bool SourceMediaManager::Configure() {
    auto rr = ac::video::ExtractRateAndResolution(format_);

    if (!output_stream_->Connect(remote_address_, sink_port1_))
        return false;

    DEBUG("dimensions: %dx%d@%d", rr.width, rr.height, rr.framerate);

    video::DisplayOutput output{video::DisplayOutput::Mode::kExtend, rr.width, rr.height, rr.framerate};

    if (!producer_->Setup(output)) {
        ERROR("Failed to setup buffer producer");
        return false;
    }

    int profile = 0, level = 0, constraint = 0;
    ac::video::ExtractProfileLevel(format_, &profile, &level, &constraint);

    auto config = encoder_->DefaultConfiguration();
    config.width = rr.width;
    config.height = rr.height;
    config.framerate = rr.framerate;
    config.profile_idc = profile;
    config.level_idc = level;
    config.constraint_set = constraint;

    if (!encoder_->Configure(config)) {
        ERROR("Failed to configure encoder");
        return false;
    }

    renderer_ = std::make_shared<ac::mir::StreamRenderer>(
                producer_, encoder_, report_factory_->CreateRendererReport());

    auto rtp_sender = std::make_shared<ac::streaming::RTPSender>(
                output_stream_, report_factory_->CreateSenderReport());
    rtp_sender->SetDelegate(shared_from_this());

    const auto mpegts_packetizer = ac::streaming::MPEGTSPacketizer::Create(
                report_factory_->CreatePacketizerReport());

    sender_ = std::make_shared<ac::streaming::MediaSender>(
                mpegts_packetizer,
                rtp_sender,
                config);

    encoder_->SetDelegate(sender_);

    pipeline_.Add(encoder_);
    pipeline_.Add(renderer_);
    pipeline_.Add(rtp_sender);
    pipeline_.Add(sender_);

    return true;
}

void SourceMediaManager::OnTransportNetworkError() {
    if (auto sp = delegate_.lock())
        sp->OnSourceNetworkError();
}

void SourceMediaManager::CancelDelayTimeout() {
    if (delay_timeout_ == 0)
        return;

    g_source_remove(delay_timeout_);
    delay_timeout_ = 0;
}

gboolean SourceMediaManager::OnStartPipeline(gpointer user_data) {
    auto thiz = static_cast<ac::WeakKeepAlive<SourceMediaManager>*>(user_data)->GetInstance().lock();
    if (!thiz)
        return FALSE;

    thiz->pipeline_.Start();
    thiz->delay_timeout_ = 0;

    return FALSE;
}

void SourceMediaManager::Play() {
    if (!IsPaused())
        return;

    DEBUG("");

    CancelDelayTimeout();

    // Deferring the actual pipeline start a bit helps to solve
    // problems with receiver devices not being ready in the same
    // timeframe as we are. If we send RTP packages to early we
    // will receive ICMP failures which are not causing much
    // issues but its better to avoid them.
    delay_timeout_ = g_timeout_add_full(G_PRIORITY_DEFAULT,
           kStreamDelayOnPlay.count(),
           &OnStartPipeline,
           new WeakKeepAlive<SourceMediaManager>(shared_from_this()),
           [](gpointer data) { delete static_cast<WeakKeepAlive<SourceMediaManager>*>(data); });

    // We defer the actual start of the pipeline a bit here but
    // stay in state 'Playing' as even if the pipeline start
    // fails we don't have any direct way yet to switch the
    // state from our position.
    state_ = State::Playing;
}

void SourceMediaManager::Pause() {
    if (IsPaused())
        return;

    CancelDelayTimeout();

    DEBUG("");

    pipeline_.Stop();

    state_ = State::Paused;
}

void SourceMediaManager::Teardown() {
    if (state_ == State::Stopped)
        return;

    DEBUG("");

    CancelDelayTimeout();

    pipeline_.Stop();

    state_ = State::Stopped;
}

bool SourceMediaManager::IsPaused() const {
    return state_ == State::Paused ||
           state_ == State::Stopped;
}

void SourceMediaManager::SendIDRPicture() {
    if (!encoder_)
        return;

    encoder_->SendIDRFrame();
}

int SourceMediaManager::GetLocalRtpPort() const {
    return sender_->LocalRTPPort();
}

} // namespace mir
} // namespace ac
