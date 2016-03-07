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

#include "mcs/logger.h"

#include "mcs/common/threadedexecutor.h"

#include "mcs/report/reportfactory.h"

#include "mcs/video/statistics.h"
#include "mcs/video/videoformat.h"

#include "mcs/streaming/mpegtspacketizer.h"
#include "mcs/streaming/rtpsender.h"

#include "mcs/mir/sourcemediamanager.h"

#include "mcs/android/h264encoder.h"

namespace mcs {
namespace mir {

SourceMediaManager::Ptr SourceMediaManager::Create(const std::string &remote_address) {
    return std::shared_ptr<SourceMediaManager>(new SourceMediaManager(remote_address));
}

SourceMediaManager::SourceMediaManager(const std::string &remote_address) :
    remote_address_(remote_address),
    state_(State::Stopped) {
}

SourceMediaManager::~SourceMediaManager() {
    if (state_ != State::Stopped)
        StopPipeline();

    mcs::video::Statistics::Instance()->Dump();
}

bool SourceMediaManager::Configure() {
    auto report_factory = report::ReportFactory::Create();
    auto rr = mcs::video::ExtractRateAndResolution(format_);

    MCS_DEBUG("dimensions: %dx%d@%d", rr.width, rr.height, rr.framerate);

    // FIXME we don't support any other mode than extend for now as that means some
    // additional work from mir to still give us properly sized frames we can hand
    // to the encoder.
    Screencast::DisplayOutput output{Screencast::DisplayMode::kExtend, rr.width, rr.height};

    connector_ = mcs::mir::Screencast::Create(output);
    if (!connector_->IsValid())
        return false;

    encoder_ = mcs::android::H264Encoder::Create(report_factory->CreateEncoderReport());

    int profile = 0, level = 0, constraint = 0;
    mcs::video::ExtractProfileLevel(format_, &profile, &level, &constraint);

    auto config = mcs::android::H264Encoder::DefaultConfiguration();
    config.width = rr.width;
    config.height = rr.height;
    config.framerate = rr.framerate;
    config.profile_idc = profile;
    config.level_idc = level;
    config.constraint_set = constraint;

    if (!encoder_->Configure(config)) {
        MCS_ERROR("Failed to configure encoder");
        return false;
    }

    encoder_executor_ = mcs::common::ThreadedExecutor::Create(encoder_, "Encoder");

    renderer_ = mcs::mir::StreamRenderer::Create(connector_, encoder_,
                                                 report_factory->CreateRendererReport());
    renderer_->SetDimensions(rr.width, rr.height);

    auto rtp_sender = mcs::streaming::RTPSender::Create(remote_address_, sink_port1_,
                                                        report_factory->CreateSenderReport());
    auto mpegts_packetizer = mcs::streaming::MPEGTSPacketizer::Create(report_factory->CreatePacketizerReport());

    sender_ = mcs::streaming::MediaSender::Create(mpegts_packetizer, rtp_sender, config);
    encoder_->SetDelegate(sender_);

    return true;
}

void SourceMediaManager::StartPipeline() {
    sender_->Start();
    encoder_executor_->Start();
    renderer_->StartThreaded();
}

void SourceMediaManager::StopPipeline() {
    renderer_->Stop();
    encoder_executor_->Stop();
    sender_->Stop();
}

void SourceMediaManager::Play() {
    if (!IsPaused() || !renderer_)
        return;

    MCS_DEBUG("");

    StartPipeline();

    state_ = State::Playing;
}

void SourceMediaManager::Pause() {
    if (IsPaused()|| !renderer_)
        return;

    MCS_DEBUG("");

    StopPipeline();

    state_ = State::Paused;
}

void SourceMediaManager::Teardown() {
    if (state_ == State::Stopped || !renderer_)
        return;

    MCS_DEBUG("");

    StopPipeline();

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
    MCS_DEBUG("local port %d", sender_->LocalRTPPort());
    return sender_->LocalRTPPort();
}

} // namespace mir
} // namespace mcs
