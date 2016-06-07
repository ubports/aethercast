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

#include "ac/logger.h"
#include "ac/basesourcemediamanager.h"
#include "ac/video/videoformat.h"

namespace {
static unsigned int next_session_id = 0;
}

namespace ac {
BaseSourceMediaManager::BaseSourceMediaManager() :
    session_id_(++next_session_id) {
}

void BaseSourceMediaManager::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

void BaseSourceMediaManager::ResetDelegate() {
    delegate_.reset();
}

wds::SessionType BaseSourceMediaManager::GetSessionType() const {
    /* Even though we will send only video for the moment in the MPEG stream,
     * we identify ourselves as an audio/video session, because some buggy
     * dongles need the audio codec to be set in the negotiation for screencast
     * to happen.
     */
    return wds::AudioVideoSession;
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

        AC_DEBUG("Video codecs supported by us:");
        for (auto c : codecs)
            ac::video::DumpVideoCodec(c);
    }

    return codecs;
}

bool BaseSourceMediaManager::InitOptimalVideoFormat(const wds::NativeVideoFormat& sink_native_format,
    const std::vector<wds::H264VideoCodec>& sink_supported_codecs) {

    AC_DEBUG("Sink native resolution:");
    ac::video::DumpNativeFormat(sink_native_format);

    AC_DEBUG("Sink supports the following codecs:");
    for (auto sink_codec : sink_supported_codecs) {
        ac::video::DumpVideoCodec(sink_codec);
    }

    bool success = false;

    format_ = wds::FindOptimalVideoFormat(sink_native_format,
                                         GetH264VideoCodecs(),
                                         sink_supported_codecs,
                                         &success);

    // Workaround buggy wds code ..
    if (format_.rate_resolution == wds::CEA1280x720p60)
        format_.rate_resolution = wds::CEA1280x720p30;

    if (!success) {
        AC_ERROR("Failed to select proper video format");
        return false;
    }

    AC_DEBUG("Found optimal video format:");
    ac::video::DumpVideoFormat(format_);

    return Configure();
}

wds::H264VideoFormat BaseSourceMediaManager::GetOptimalVideoFormat() const {
    return format_;
}

bool BaseSourceMediaManager::InitOptimalAudioFormat(const std::vector<wds::AudioCodec>& sink_codecs) {
    if (sink_codecs.empty())
        return false;

    /* Just take first codec until we implement audio */
    audio_codec_ = sink_codecs[0];
    return true;
}

wds::AudioCodec BaseSourceMediaManager::GetOptimalAudioFormat() const {
  return audio_codec_;
}

void BaseSourceMediaManager::SendIDRPicture() {
    AC_WARNING("Unimplemented IDR picture request");
}

std::string BaseSourceMediaManager::GetSessionId() const {
    return std::to_string(session_id_);
}
} // namespace ac
