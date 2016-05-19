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

#include "ac/video/videoformat.h"
#include "ac/logger.h"

namespace ac {
namespace video {

std::string ResolutionTypeToString(wds::ResolutionType type) {
    switch (type) {
    case wds::CEA:
        return "CEA";
    case wds::VESA:
        return "VESA";
    case wds::HH:
        return "HH";
    default:
        break;
    }
    return "unknown";
}

std::string CEARatesAndResolutionsToString(wds::CEARatesAndResolutions type) {
    switch (type) {
    case wds::CEA640x480p60:
        return "640x480p60";
    case wds::CEA720x480p60:
        return "720x480p60";
    case wds::CEA720x480i60:
        return "720x480i60";
    case wds::CEA720x576p50:
        return "720x576p50";
    case wds::CEA720x576i50:
        return "720x576i50";
    case wds::CEA1280x720p30:
        return "1280x720p30";
    case wds::CEA1280x720p60:
        return "1280x720p60";
    case wds::CEA1920x1080p30:
        return "1920x1080p30";
    case wds::CEA1920x1080p60:
        return "1920x1080p60";
    case wds::CEA1920x1080i60:
        return "1920x1080i60";
    case wds::CEA1280x720p25:
        return "1280x720p25";
    case wds::CEA1280x720p50:
        return "1280x720p50";
    case wds::CEA1920x1080p25:
        return "1920x1080p25";
    case wds::CEA1920x1080p50:
        return "1920x1080p50";
    case wds::CEA1920x1080i50:
        return "1920x1080i50";
    case wds::CEA1280x720p24:
        return "1280x720p24";
    case wds::CEA1920x1080p24:
        return "1920x1080p24";
    default:
        break;
    }
    return "unknown";
}

std::string VESARatesAndResolutionsToString(wds::VESARatesAndResolutions type) {
    switch (type) {
    case wds::VESA800x600p30:
        return "800x600p30";
    case wds::VESA800x600p60:
        return "800x600p60";
    case wds::VESA1024x768p30:
        return "1024x768p30";
    case wds::VESA1024x768p60:
        return "1024x768p60";
    case wds::VESA1152x864p30:
        return "1152x864p30";
    case wds::VESA1152x864p60:
        return "1152x864p60";
    case wds::VESA1280x768p30:
        return "1280x768p30";
    case wds::VESA1280x768p60:
        return "1280x768p60";
    case wds::VESA1280x800p30:
        return "1280x800p30";
    case wds::VESA1280x800p60:
        return "1280x800p60";
    case wds::VESA1360x768p30:
        return "1360x768p30";
    case wds::VESA1360x768p60:
        return "1360x768p60";
    case wds::VESA1366x768p30:
        return "1366x768p30";
    case wds::VESA1366x768p60:
        return "1366x768p60";
    case wds::VESA1280x1024p30:
        return "1280x1024p30";
    case wds::VESA1280x1024p60:
        return "1280x1024p60";
    case wds::VESA1400x1050p30:
        return "1400x1050p30";
    case wds::VESA1400x1050p60:
        return "1400x1050p60";
    case wds::VESA1440x900p30:
        return "1440x900p30";
    case wds::VESA1440x900p60:
        return "1440x900p60";
    case wds::VESA1600x900p30:
        return "1600x900p30";
    case wds::VESA1600x900p60:
        return "1600x900p60";
    case wds::VESA1600x1200p30:
        return "1600x1200p30";
    case wds::VESA1600x1200p60:
        return "1600x1200p60";
    case wds::VESA1680x1024p30:
        return "1680x1024p30";
    case wds::VESA1680x1024p60:
        return "1680x1024p60";
    case wds::VESA1680x1050p30:
        return "1680x1050p30";
    case wds::VESA1680x1050p60:
        return "1680x1050p60";
    case wds::VESA1920x1200p30:
        return "1920x1200p30";
    default:
        break;
    }
    return "unknown";
}

std::string HHRatesAndResolutionsToString(wds::HHRatesAndResolutions type) {
    switch (type) {
    case wds::HH800x480p30:
      return "800x480p30";
    case wds::HH800x480p60:
      return "800x480p60";
    case wds::HH854x480p30:
      return "854x480p30";
    case wds::HH854x480p60:
      return "854x480p60";
    case wds::HH864x480p30:
      return "864x480p30";
    case wds::HH864x480p60:
      return "864x480p60";
    case wds::HH640x360p30:
      return "640x360p30";
    case wds::HH640x360p60:
      return "640x360p60";
    case wds::HH960x540p30:
      return "960x540p30";
    case wds::HH960x540p60:
      return "960x540p60";
    case wds::HH848x480p30:
      return "848x480p30";
    case wds::HH848x480p60:
      return "848x480p60";
    default:
        break;
    }
    return "unknown";
}

std::string LevelToString(wds::H264Level level) {
    switch (level) {
    case wds::k3_1:
        return "3.1";
    case wds::k3_2:
        return "3.2";
    case wds::k4:
        return "4";
    case wds::k4_1:
        return "4.1";
    case wds::k4_2:
        return "4.2";
    default:
        break;
    }
    return "unknown";
}

std::string ProfileToString(wds::H264Profile profile) {
    switch (profile) {
    case wds::CBP:
        return "cbp";
    case wds::CHP:
        return "chp";
    default:
        break;
    }
    return "unknown";
}

void DumpVideoCodec(const wds::H264VideoCodec &codec) {
    int i = 0;

    DEBUG(" profile: %s", ProfileToString(codec.profile));
    DEBUG(" level: %s", LevelToString(codec.level));

    DEBUG(" CEA resolutions: ");
    for (i = wds::CEA640x480p60; i <= wds::CEA1920x1080p24; ++i)
        if (codec.cea_rr.test(i))
            DEBUG("  %s", CEARatesAndResolutionsToString(static_cast<wds::CEARatesAndResolutions>(i)).c_str());

    DEBUG(" VESA resolutions: ");
    for (i = wds::VESA800x600p30; i <= wds::VESA1920x1200p30; ++i)
        if (codec.vesa_rr.test(i))
            DEBUG("  %s", VESARatesAndResolutionsToString(static_cast<wds::VESARatesAndResolutions>(i)).c_str());

    DEBUG(" HH resolutions: ");
    for (i = wds::HH800x480p30; i <= wds::HH848x480p60; ++i)
        if (codec.hh_rr.test(i))
            DEBUG("  %s", HHRatesAndResolutionsToString(static_cast<wds::HHRatesAndResolutions>(i)).c_str());
}

void DumpVideoFormat(const wds::H264VideoFormat &format) {
    DEBUG(" profile: %s", ProfileToString(format.profile));
    DEBUG(" level: %s", LevelToString(format.level));
    DEBUG(" resolution type: %s", ResolutionTypeToString(format.type));

    if (format.type == wds::CEA)
        DEBUG("resolution: %s", CEARatesAndResolutionsToString(static_cast<wds::CEARatesAndResolutions>(format.rate_resolution)));
    if (format.type == wds::VESA)
        DEBUG("resolution: %s", VESARatesAndResolutionsToString(static_cast<wds::VESARatesAndResolutions>(format.rate_resolution)));
    if (format.type == wds::HH)
        DEBUG("resolution: %s", HHRatesAndResolutionsToString(static_cast<wds::HHRatesAndResolutions>(format.rate_resolution)));
}

void DumpNativeFormat(const wds::NativeVideoFormat &format) {
    DEBUG(" resolution type: %s", ResolutionTypeToString(format.type));

    if (format.type == wds::CEA)
        DEBUG("resolution: %s", CEARatesAndResolutionsToString((static_cast<wds::CEARatesAndResolutions>(format.rate_resolution))));
    if (format.type == wds::VESA)
        DEBUG("resolution: %s", VESARatesAndResolutionsToString(static_cast<wds::VESARatesAndResolutions>(format.rate_resolution)));
    if (format.type == wds::HH)
        DEBUG("resolution: %s", HHRatesAndResolutionsToString(static_cast<wds::HHRatesAndResolutions>(format.rate_resolution)));
}

std::string ExtractH264Profile(const wds::H264VideoFormat &format) {
    std::string profile;
    switch (format.profile) {
    case wds::CBP:
        profile = "constrained-baseline";
        break;
    case wds::CHP:
        profile = "high";
        break;
    default:
        break;
    }
    return profile;
}

std::string ExtractH264Level(const wds::H264VideoFormat &format) {
    std::string level;
    switch (format.level) {
    case wds::k3_1:
        level = "3.1";
        break;
    case wds::k3_2:
        level = "3.2";
        break;
    case wds::k4:
        level = "4";
        break;
    case wds::k4_1:
        level = "4.1";
        break;
    case wds::k4_2:
        level = "4.2";
        break;
    default:
        break;
    }
    return level;
}

RateAndResolution ExtractRateAndResolution(const wds::H264VideoFormat &format) {
    RateAndResolution rr;
    switch (format.type) {
    case wds::CEA:
        switch (format.rate_resolution) {
        case wds::CEA640x480p60:
            rr.width = 640;
            rr.height = 480;
            rr.framerate = 60;
            break;
        case wds::CEA720x480p60:
        case wds::CEA720x480i60:
            rr.width = 720;
            rr.height = 480;
            rr.framerate = 60;
            break;
        case wds::CEA720x576p50:
        case wds::CEA720x576i50:
            rr.width = 720;
            rr.height = 576;
            rr.framerate = 50;
            break;
        case wds::CEA1280x720p30:
            rr.width = 1280;
            rr.height = 720;
            rr.framerate = 30;
            break;
        case wds::CEA1280x720p60:
            rr.width = 1280;
            rr.height = 720;
            rr.framerate = 60;
            break;
        case wds::CEA1920x1080p30:
            rr.width = 1920;
            rr.height = 1080;
            rr.framerate = 30;
            break;
        case wds::CEA1920x1080p60:
        case wds::CEA1920x1080i60:
            rr.width = 1920;
            rr.height = 1080;
            rr.framerate = 60;
            break;
        case wds::CEA1280x720p25:
            rr.width = 1280;
            rr.height = 720;
            rr.framerate = 25;
            break;
        case wds::CEA1280x720p50:
            rr.width = 1280;
            rr.height = 720;
            rr.framerate = 50;
            break;
        case wds::CEA1280x720p24:
            rr.width = 1280;
            rr.height = 720;
            rr.framerate = 24;
            break;
        case wds::CEA1920x1080p25:
            rr.width = 1920;
            rr.height = 1080;
            rr.framerate = 25;
            break;
        case wds::CEA1920x1080p50:
            rr.width = 1920;
            rr.height = 1080;
            rr.framerate = 50;
            break;
        case wds::CEA1920x1080i50:
            rr.width = 1920;
            rr.height = 1080;
            rr.framerate = 50;
            break;
        case wds::CEA1920x1080p24:
            rr.width = 1920;
            rr.height = 1080;
            rr.framerate = 24;
            break;
        default:
            break;
        }
        break;
    // FIXME Add support for VESA and HH
    default:
        rr.width = 640;
        rr.height = 480;
        rr.framerate = 30;
        break;
    }
    return rr;
}

void ExtractProfileLevel(const wds::H264VideoFormat &format, int *profile,
                            int *level, int *constraint) {

    if (format.profile == wds::CBP) {
        *profile = 66;
        *constraint = 0xc0;
    }
    else if (format.profile == wds::CHP) {
        *profile = 100;
        *constraint = 0x0c;
    }

    switch (format.level) {
    case wds::k3_1:
        *level = 31;
        break;
    case wds::k3_2:
        *level = 32;
        break;
    case wds::k4:
        *level = 40;
        break;
    case wds::k4_1:
        *level = 41;
        break;
    case wds::k4_2:
        *level = 42;
    default:
        break;
    }
}

} // namespace video
} // namespace ac
