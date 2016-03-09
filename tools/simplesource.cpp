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

#include "mcs/utils.h"
#include "mcs/mediamanagerfactory.h"

#include "simplesource.h"

namespace mcs {
namespace tools {

SimpleSource::Ptr SimpleSource::Create(const std::string &remote_address, int port) {
    return std::shared_ptr<SimpleSource>(new SimpleSource(remote_address, port));
}

SimpleSource::SimpleSource(const std::string &remote_address, int port) :
    media_manager_(mcs::MediaManagerFactory::CreateSource(remote_address)) {

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
    // android supports. But for now we just consider CBP/CHP with level 3.2
    // as that is the same Android configures its setup with.
    wds::H264VideoCodec codec1(wds::CBP, wds::k3_2, cea_rr, vesa_rr, hh_rr);
    sink_codecs_.push_back(codec1);
    wds::H264VideoCodec codec2(wds::CHP, wds::k3_2, cea_rr, vesa_rr, hh_rr);
    sink_codecs_.push_back(codec2);

    sink_native_format_.type = wds::CEA;
    sink_native_format_.rate_resolution = wds::CEA1280x720p30;

    media_manager_->SetSinkRtpPorts(port, 0);
}

SimpleSource::~SimpleSource() {
    Stop();
}

void SimpleSource::Start() {
    media_manager_->InitOptimalVideoFormat(sink_native_format_,
                                           sink_codecs_);
    media_manager_->Play();
}

void SimpleSource::Stop() {
    media_manager_->Teardown();
}

} // namespace tools
} // namespace mcs
