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

#include <hybris/properties/properties.h>

#include "ac/logger.h"
#include "ac/mediamanagerfactory.h"
#include "ac/utils.h"
#include "ac/logger.h"

#include "ac/mir/sourcemediamanager.h"

#include "ac/common/threadedexecutorfactory.h"

#include "ac/report/reportfactory.h"

#include "ac/network/udpstream.h"

#include "ac/android/h264encoder.h"
#include "ac/gst/h264encoder.h"
#include "ac/droidmedia/h264encoder.h"

namespace ac {

void NullSourceMediaManager::Play() {
    AC_WARNING("NullSourceMediaManager: Not implemented");
}

void NullSourceMediaManager::Pause() {
    AC_WARNING("NullSourceMediaManager: Not implemented");
}

void NullSourceMediaManager::Teardown() {
    AC_WARNING("NullSourceMediaManager: Not implemented");
}

bool NullSourceMediaManager::IsPaused() const {
    AC_WARNING("NullSourceMediaManager: Not implemented");
    return false;
}

bool NullSourceMediaManager::Configure() {
    AC_WARNING("NullSourceMediaManager: Not implemented");
    return false;
}

std::shared_ptr<BaseSourceMediaManager> MediaManagerFactory::CreateSource(const std::string &remote_address,
                                                                          const ac::network::Stream::Ptr &output_stream) {
    std::string type = Utils::GetEnvValue("MIRACAST_SOURCE_TYPE");
    if (type.length() == 0)
        type = "mir";

    AC_DEBUG("Creating source media manager of type %s", type.c_str());

    if (type == "mir") {
        // Shipped encoders: "", "droidmedia", "gst"
        // Effectively only the legacy and droidmedia are tested
        std::string encoder_name = "";
        char encoder_prop[PROP_VALUE_MAX];
        property_get("ubuntu.widi.encoder", encoder_prop, "");
        if (strlen(encoder_prop) > 0) {
            encoder_name = std::string(encoder_prop);
        }

        // Decide whether to copy pixels into main memory
        bool readout = false;
        char readout_prop[PROP_VALUE_MAX];
        property_get("ubuntu.widi.readout", readout_prop, "");
        if (strlen(readout_prop) > 0 && !strcmp(readout_prop, "true")) {
            readout = true;
        }

        const auto executor_factory = std::make_shared<common::ThreadedExecutorFactory>();
        const auto report_factory = report::ReportFactory::Create();
        const auto screencast = std::make_shared<ac::mir::Screencast>(readout);
        const auto encoder = 
            (encoder_name == "gst") ? ac::gst::H264Encoder::Create(report_factory->CreateEncoderReport()) :
            (encoder_name == "droidmedia") ? ac::droidmedia::H264Encoder::Create(report_factory->CreateEncoderReport()) :
            ac::android::H264Encoder::Create(report_factory->CreateEncoderReport(), readout);

        return std::make_shared<ac::mir::SourceMediaManager>(
                    remote_address,
                    executor_factory,
                    screencast,
                    encoder,
                    output_stream,
                    report_factory);
    }

    return std::make_shared<NullSourceMediaManager>();
}
} // namespace ac
