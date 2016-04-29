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

#include <boost/concept_check.hpp>

#include "mcs/logger.h"
#include "mcs/mir/screencast.h"

namespace {
static constexpr const char *kMirSocket{"/run/mir_socket"};
static constexpr const char *kMirConnectionName{"aethercast screencast client"};
}

namespace mcs {
namespace mir {

Screencast::Screencast() :
    connection_(nullptr),
    screencast_(nullptr),
    buffer_stream_(nullptr) {
}

Screencast::~Screencast() {
    MCS_DEBUG("");

    if (screencast_)
        mir_screencast_release_sync(screencast_);

    if (connection_)
        mir_connection_release(connection_);
}

bool Screencast::Setup(const video::DisplayOutput &output) {
    if (screencast_ || connection_ || buffer_stream_)
        return false;

    if (output.mode != video::DisplayOutput::Mode::kExtend) {
        MCS_ERROR("Unsupported display output mode specified '%s'", output.mode);
        return false;
    }

    MCS_DEBUG("Setting up screencast [%s %dx%d]", output.mode,
              output.width, output.height);

    connection_ = mir_connect_sync(kMirSocket, kMirConnectionName);
    if (!mir_connection_is_valid(connection_)) {
        MCS_ERROR("Failed to connect to Mir server: %s",
                  mir_connection_get_error_message(connection_));
        return false;
    }

    const auto config = mir_connection_create_display_config(connection_);
    if (!config) {
        MCS_ERROR("Failed to create display configuration: %s",
                  mir_connection_get_error_message(connection_));
        return false;
    }

    MirDisplayOutput *active_output = nullptr;
    unsigned int output_index = 0;

    for (unsigned int i = 0; i < config->num_outputs; ++i) {
        if (config->outputs[i].connected &&
            config->outputs[i].used &&
            config->outputs[i].current_mode < config->outputs[i].num_modes) {
            // Found an active connection we can just use for our purpose
            active_output = &config->outputs[i];
            output_index = i;
            break;
        }
    }

    if (!active_output) {
        MCS_ERROR("Failed to find a suitable display output");
        return false;
    }

    const MirDisplayMode *display_mode = &active_output->modes[active_output->current_mode];

    auto spec = mir_create_screencast_spec(connection_);
    if (!spec) {
        ERROR("Failed to create Mir screencast specification: %s",
              mir_screencast_get_error_message(screencast_));
        return false;
    }

    mir_screencast_spec_set_width(spec, output.width);
    mir_screencast_spec_set_height(spec, output.height);

    MirRectangle region;
    // If we request a screen region outside the available screen area
    // mir will create a mir output which is then available for everyone
    // as just another display.
    region.left = display_mode->horizontal_resolution;
    region.top = 0;
    region.width = display_mode->vertical_resolution;
    region.height = display_mode->horizontal_resolution;

    mir_screencast_spec_set_capture_region(spec, &region);

    output_.refresh_rate = display_mode->refresh_rate;

    MCS_INFO("Selected output ID %i [(%ix%i)+(%ix%i)] orientation %d",
             output_index,
             display_mode->vertical_resolution,
             display_mode->horizontal_resolution,
             region.left, region.top,
             active_output->orientation);

    unsigned int num_pixel_formats = 0;
    MirPixelFormat pixel_format;
    mir_connection_get_available_surface_formats(connection_, &pixel_format,
                                                 1, &num_pixel_formats);
    if (num_pixel_formats == 0) {
        MCS_ERROR("Failed to find suitable pixel format: %s",
                  mir_connection_get_error_message(connection_));
        return false;
    }

    mir_screencast_spec_set_pixel_format(spec, pixel_format);
    mir_screencast_spec_set_mirror_mode(spec, mir_mirror_mode_vertical);
    mir_screencast_spec_set_number_of_buffers(spec, 2);

    screencast_ = mir_screencast_create_sync(spec);
    mir_screencast_spec_release(spec);
    if (!mir_screencast_is_valid(screencast_)) {
        ERROR("Failed to create Mir screencast: %s",
              mir_screencast_get_error_message(screencast_));
        return false;
    }

    buffer_stream_ = mir_screencast_get_buffer_stream(screencast_);
    if (!buffer_stream_) {
        MCS_ERROR("Failed to setup Mir buffer stream");
        return false;
    }

    output_ = output;

    return true;
}

void Screencast::SwapBuffers() {
    if (!buffer_stream_)
        return;

    mir_buffer_stream_swap_buffers_sync(buffer_stream_);
}

video::DisplayOutput Screencast::OutputMode() const {
    return output_;
}

void* Screencast::CurrentBuffer() const {
    if (!buffer_stream_)
        return nullptr;

    MirNativeBuffer *buffer = nullptr;
    mir_buffer_stream_get_current_buffer(buffer_stream_, &buffer);
    return reinterpret_cast<void*>(buffer);
}
} // namespace mir
} // namespace mcs
