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

#include <QDebug>

#include "basesourcemediamanager.h"

wds::SessionType BaseSourceMediaManager::GetSessionType() const
{
    return wds::VideoSession;
}

void BaseSourceMediaManager::SetSinkRtpPorts(int port1, int port2)
{
    sinkPort1 = port1;
    sinkPort2 = port2;
}

std::pair<int, int> BaseSourceMediaManager::GetSinkRtpPorts() const
{
    return std::pair<int, int>(sinkPort1, sinkPort2);
}

int BaseSourceMediaManager::GetLocalRtpPort() const
{
    return sinkPort1;
}

std::vector<wds::H264VideoCodec> GetH264VideoCodecs()
{
    static std::vector<wds::H264VideoCodec> codecs;
    if (codecs.empty()) {
        wds::RateAndResolutionsBitmap cea_rr;
        wds::RateAndResolutionsBitmap vesa_rr;
        wds::RateAndResolutionsBitmap hh_rr;
        wds::RateAndResolution i;
        // declare that we support all resolutions, CHP and level 4.2
        // gstreamer should handle all of it :)
        for (i = wds::CEA640x480p60; i <= wds::CEA1920x1080p24; ++i)
            cea_rr.set(i);
        for (i = wds::VESA800x600p30; i <= wds::VESA1920x1200p30; ++i)
            vesa_rr.set(i);
        for (i = wds::HH800x480p30; i <= wds::HH848x480p60; ++i)
            hh_rr.set(i);

        wds::H264VideoCodec codec(wds::CHP, wds::k4_2, cea_rr, vesa_rr, hh_rr);
        codecs.push_back(codec);
    }

    return codecs;
}

bool BaseSourceMediaManager::InitOptimalVideoFormat(const wds::NativeVideoFormat& sink_native_format,
    const std::vector<wds::H264VideoCodec>& sink_supported_codecs)
{

    format = wds::FindOptimalVideoFormat(sink_native_format,
                                         GetH264VideoCodecs(),
                                         sink_supported_codecs);

    qWarning() << "Found optimal video format:"
               << "profile: " << format.profile
               << "level: " << format.level
               << "res type: " << format.type
               << "rate & resolution: " << format.rate_resolution;

    return true;
}

wds::H264VideoFormat BaseSourceMediaManager::GetOptimalVideoFormat() const
{
    return format;
}

bool BaseSourceMediaManager::InitOptimalAudioFormat(const std::vector<wds::AudioCodec>& sink_codecs)
{
    for (const auto& codec : sink_codecs) {
        if (codec.format == wds::AAC && codec.modes.test(wds::AAC_48K_16B_2CH))
            return true;
    }

    return false;
}

wds::AudioCodec BaseSourceMediaManager::GetOptimalAudioFormat() const
{
  wds::AudioModes audio_modes;
  audio_modes.set(wds::AAC_48K_16B_2CH);

  return wds::AudioCodec(wds::AAC, audio_modes, 0);
}

void BaseSourceMediaManager::SendIDRPicture()
{
    qWarning() << "Unimplemented IDR picture request";
}
