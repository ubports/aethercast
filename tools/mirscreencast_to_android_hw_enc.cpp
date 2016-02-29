/*
 *  Copyright (C) 2016 Canonical, Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <memory>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

#include <mcs/logger.h>
#include <mcs/mir/screencast.h>
#include <mcs/mir/streamrenderer.h>
#include <mcs/android/h264encoder.h>

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;

    mcs::mir::Screencast::DisplayOutput output{mcs::mir::Screencast::DisplayMode::kMirror, 0, 0, 0};

    auto connector = mcs::mir::Screencast::Create(output);
    if (!connector->IsValid()) {
        MCS_ERROR("Failed to setup Mir stream connector");
        return -EIO;
    }

    auto mode = connector->OutputMode();

    auto encoder = mcs::android::H264Encoder::Create();

    auto config = mcs::android::H264Encoder::DefaultConfiguration();
    config.width = mode.width;
    config.height = mode.height;
    config.framerate = 60;

    encoder->Configure(config);
    encoder->Start();

    auto renderer = mcs::mir::StreamRenderer::Create(connector, encoder);
    renderer->StartThreaded();

    std::this_thread::sleep_for(std::chrono::seconds{30});

    renderer->Stop();
    encoder->Stop();

    return 0;
}
