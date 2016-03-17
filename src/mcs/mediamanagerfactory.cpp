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

#include <string.h>

#include "logger.h"
#include "mediamanagerfactory.h"
#include "mir/sourcemediamanager.h"
#include "x11sourcemediamanager.h"
#include "testsourcemediamanager.h"
#include "utils.h"
#include "logging.h"

#include "mcs/report/reportfactory.h"

#include "mcs/network/udpstream.h"

#include "mcs/android/h264encoder.h"

namespace mcs {

void NullSourceMediaManager::Play() {
    WARNING("NullSourceMediaManager: Not implemented");
}

void NullSourceMediaManager::Pause() {
    WARNING("NullSourceMediaManager: Not implemented");
}

void NullSourceMediaManager::Teardown() {
    WARNING("NullSourceMediaManager: Not implemented");
}

bool NullSourceMediaManager::IsPaused() const {
    WARNING("NullSourceMediaManager: Not implemented");
    return false;
}

bool NullSourceMediaManager::Configure() {
    WARNING("NullSourceMediaManager: Not implemented");
    return false;
}

std::shared_ptr<BaseSourceMediaManager> MediaManagerFactory::CreateSource(const std::string &remote_address) {
    std::string type = Utils::GetEnvValue("MIRACAST_SOURCE_TYPE");
    if (type.length() == 0)
        type = "mir";

    DEBUG("Creating source media manager of type %s", type.c_str());

    if (type == "mir") {
        auto report_factory = report::ReportFactory::Create();
        auto screencast = std::make_shared<mcs::mir::Screencast>();
        auto encoder = mcs::android::H264Encoder::Create(report_factory->CreateEncoderReport());
        auto network_stream = std::make_shared<mcs::network::UdpStream>();

        return std::make_shared<mcs::mir::SourceMediaManager>(
                    remote_address,
                    screencast,
                    encoder,
                    network_stream,
                    report_factory);
    }
    else if (type == "x11")
        return X11SourceMediaManager::create(remote_address);
    else if (type == "test")
        return TestSourceMediaManager::create(remote_address);

    return std::make_shared<NullSourceMediaManager>();
}
} // namespace mcs
