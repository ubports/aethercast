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
#include "mcs/mir/streamconnector.h"

namespace {
static constexpr const char *kMirSocket{"/run/mir_socket"};
static constexpr const char *kMirConnectionName{"aethercast screencast client"};
}

namespace mcs {
namespace mir {

std::string StreamConnector::DisplayModeToString(const DisplayMode &mode) {
    switch (mode) {
    case DisplayMode::kExtend:
        return "extend";
    case DisplayMode::kMirror:
        return "mirror";
    default:
        break;
    }
    return "unknown";
}

StreamConnector::Ptr StreamConnector::Create(const StreamConnector::DisplayOutput &output) {
    return std::shared_ptr<StreamConnector>(new StreamConnector(output));
}

StreamConnector::StreamConnector(const StreamConnector::DisplayOutput &output) :
    output_(output) {
    connection_ = mir_connect_sync(kMirSocket, kMirConnectionName);
    if (!mir_connection_is_valid(connection_)) {
        MCS_ERROR("Failed to connect to Mir server: %s",
                  mir_connection_get_error_message(connection_));
        return;
    }

    auto config = mir_connection_create_display_config(connection_);

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
        return;
    }

    const MirDisplayMode *display_mode = &active_output->modes[active_output->current_mode];

    params_.height = display_mode->vertical_resolution;
    params_.width = display_mode->horizontal_resolution;

    if (output_.mode == DisplayMode::kMirror) {
        params_.region.left = 0;
        params_.region.top = 0;
        params_.region.width = params_.width;
        params_.region.height = params_.height;

        output_.width = params_.width;
        output_.height = params_.height;
    }
    else if (output_.mode == DisplayMode::kExtend) {
        // If we request a screen region outside the available screen area
        // mir will create a mir output which is then available for everyone
        // as just another display.
        params_.region.left = params_.width;
        params_.region.top = 0;
        params_.region.width = output_.width;
        params_.region.height = output_.height;

        params_.width = output_.width;
        params_.height = output_.height;
    }

    output_.refresh_rate = display_mode->refresh_rate;

    MCS_INFO("Selected output ID %i [(%ix%i)+(%ix%i)] orientation %d",
             output_index,
             params_.width, params_.height,
             params_.region.left, params_.region.top,
             active_output->orientation);

    MCS_DEBUG("Setting up screencast [%s %dx%d]",
              DisplayModeToString(output_.mode),
              output_.width,
              output_.height);

    unsigned int num_pixel_formats = 0;
    mir_connection_get_available_surface_formats(connection_, &params_.pixel_format,
                                                 1, &num_pixel_formats);
    if (num_pixel_formats == 0) {
        MCS_ERROR("Failed to find suitable pixel format: %s",
                  mir_connection_get_error_message(connection_));
        return;
    }

    screencast_ = mir_connection_create_screencast_sync(connection_, &params_);
    if (!screencast_) {
        MCS_ERROR("Failed to create Mir screencast: %s",
                  mir_connection_get_error_message(connection_));
        return;
    }

    buffer_stream_ = mir_screencast_get_buffer_stream(screencast_);
    if (!buffer_stream_) {
        MCS_ERROR("Failed to setup Mir buffer stream");
        return;
    }

    auto platform_type = mir_buffer_stream_get_platform_type(buffer_stream_);
    if (platform_type != mir_platform_type_android) {
        MCS_ERROR("Not running with android platform: This is not supported.");
        mir_buffer_stream_release_sync(buffer_stream_);
        buffer_stream_ = nullptr;
        return;
    }
}

StreamConnector::~StreamConnector() {
    if (screencast_)
        mir_screencast_release_sync(screencast_);

    if (connection_)
        mir_connection_release(connection_);
}

void StreamConnector::SwapBuffersSync() {
    if (!buffer_stream_)
        return;

    mir_buffer_stream_swap_buffers_sync(buffer_stream_);
}

void StreamConnector::SwapBuffers() {
    if (!buffer_stream_)
        return;

    mir_buffer_stream_swap_buffers(buffer_stream_, [](MirBufferStream *stream, void *client_context) {
        boost::ignore_unused_variable_warning(stream);
        boost::ignore_unused_variable_warning(client_context);

        MCS_DEBUG("Buffers are swapped now");

    }, nullptr);
}

bool StreamConnector::IsValid() const {
    return connection_ && screencast_ &&  buffer_stream_;
}

void* StreamConnector::NativeWindowHandle() const {
    if (!buffer_stream_)
        return nullptr;

    return reinterpret_cast<void*>(mir_buffer_stream_get_egl_native_window(buffer_stream_));
}

void* StreamConnector::NativeDisplayHandle() const {
    if (!connection_)
        return nullptr;

    return mir_connection_get_egl_native_display(connection_);
}

StreamConnector::DisplayOutput StreamConnector::OutputMode() const {
    return output_;
}

MirNativeBuffer* StreamConnector::CurrentBuffer() const {
    MirNativeBuffer *buffer = nullptr;
    mir_buffer_stream_get_current_buffer(buffer_stream_, &buffer);
    return buffer;
}
} // namespace mir
} // namespace mcs
