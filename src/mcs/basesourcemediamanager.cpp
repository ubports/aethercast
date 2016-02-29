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

#include <iostream>

#include <glib.h>

#include "mcs/logger.h"
#include "mcs/basesourcemediamanager.h"
#include "mcs/video/videoformat.h"

namespace {
static unsigned int next_session_id = 0;
}

namespace {
static unsigned int next_session_id = 0;
}

namespace mcs {
BaseSourceMediaManager::BaseSourceMediaManager() :
    session_id_(++next_session_id) {
}

wds::SessionType BaseSourceMediaManager::GetSessionType() const {
    return wds::VideoSession;
}

void BaseSourceMediaManager::SetSinkRtpPorts(int port1, int port2) {
    sink_port1_ = port1;
    sink_port2_ = port2;
}

std::pair<int, int> BaseSourceMediaManager::GetSinkRtpPorts() const {
    return std::pair<int, int>(sink_port1_, sink_port2_);
}

int BaseSourceMediaManager::GetLocalRtpPort() const {
    return sink_port1_;
}

std::vector<wds::H264VideoCodec> BaseSourceMediaManager::GetH264VideoCodecs() {
    static std::vector<wds::H264VideoCodec> codecs;
    if (codecs.empty()) {
        wds::RateAndResolutionsBitmap cea_rr;
        wds::RateAndResolutionsBitmap vesa_rr;
        wds::RateAndResolutionsBitmap hh_rr;

        // We only support 720p here for now as that is our best performing
        // resolution with regard of all other bits in the pipeline. Eventually
        // we will add 60 Hz here too but for now only everything up to 30 Hz.
        cea_rr.set(wds::CEA1280x720p30);
        cea_rr.set(wds::CEA1280x720p25);
        cea_rr.set(wds::CEA1280x720p24);

        // FIXME which profiles and formats we support highly depends on what
        // android supports. But for now we just consider CBP with level 3.1
        // as that is the same Android configures its setup with.
        wds::H264VideoCodec codec1(wds::CBP, wds::k3_1, cea_rr, vesa_rr, hh_rr);
        codecs.push_back(codec1);

        DEBUG("Video codecs supported by us:");
        for (auto c : codecs)
            mcs::video::DumpVideoCodec(c);
    }

    return codecs;
}

bool BaseSourceMediaManager::InitOptimalVideoFormat(const wds::NativeVideoFormat& sink_native_format,
    const std::vector<wds::H264VideoCodec>& sink_supported_codecs) {

    DEBUG("Sink native resolution:");
    mcs::video::DumpNativeFormat(sink_native_format);

    DEBUG("Sink supports the following codecs:");
    for (auto sink_codec : sink_supported_codecs) {
        mcs::video::DumpVideoCodec(sink_codec);
    }

    bool success = false;

    format_ = wds::FindOptimalVideoFormat(sink_native_format,
                                         GetH264VideoCodecs(),
                                         sink_supported_codecs,
                                         &success);

    // Workaround buggy wds code ..
    if (format_.rate_resolution == wds::CEA1280x720p60)
        format_.rate_resolution = wds::CEA1280x720p30;

    if (!success)
        MCS_WARNING("Failed to select proper video format");

    DEBUG("Found optimal video format:");
    mcs::video::DumpVideoFormat(format_);

    return Configure();
}

wds::H264VideoFormat BaseSourceMediaManager::GetOptimalVideoFormat() const {
    return format_;
}

bool BaseSourceMediaManager::InitOptimalAudioFormat(const std::vector<wds::AudioCodec>& sink_codecs) {
    for (const auto& codec : sink_codecs) {
        if (codec.format == wds::AAC && codec.modes.test(wds::AAC_48K_16B_2CH))
            return true;
    }

    return false;
}

wds::AudioCodec BaseSourceMediaManager::GetOptimalAudioFormat() const {
  wds::AudioModes audio_modes;
  audio_modes.set(wds::AAC_48K_16B_2CH);

  return wds::AudioCodec(wds::AAC, audio_modes, 0);
}

void BaseSourceMediaManager::SendIDRPicture() {
    WARNING("Unimplemented IDR picture request");
}

std::string BaseSourceMediaManager::GetSessionId() const {
    return mcs::Utils::Sprintf("%d", session_id_);
}
} // namespace mcs
