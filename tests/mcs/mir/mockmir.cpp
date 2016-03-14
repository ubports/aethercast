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

#include "mockmir.h"

namespace {
mcs::test::mir::MockMir *global_mock = nullptr;
}

namespace mcs {
namespace test {
namespace mir {

MockMir::MockMir() {
    using namespace testing;
    assert(global_mock == NULL && "Only one mock object per process is allowed");

    global_mock = this;
}

MockMir::~MockMir() {
    global_mock = nullptr;
}

} // namespace mir
} // namespace tests
} // namespace mcs

MirConnection *mir_connect_sync(char const *server, char const *app_name) {
    return global_mock->mir_connect_sync(server, app_name);
}

bool mir_connection_is_valid(MirConnection *connection) {
    return global_mock->mir_connection_is_valid(connection);
}

void mir_connection_release(MirConnection *connection) {
    global_mock->mir_connection_release(connection);
}

char const *mir_connection_get_error_message(MirConnection *connection) {
    return global_mock->mir_connection_get_error_message(connection);
}

MirDisplayConfiguration* mir_connection_create_display_config(MirConnection *connection) {
    return global_mock->mir_connection_create_display_config(connection);
}

void mir_connection_get_available_surface_formats(
    MirConnection* connection, MirPixelFormat* formats,
    unsigned const int format_size, unsigned int *num_valid_formats) {
    global_mock->mir_connection_get_available_surface_formats(connection, formats, format_size, num_valid_formats);
}

MirScreencast *mir_connection_create_screencast_sync(
    MirConnection *connection,
    MirScreencastParameters *parameters) {
    return global_mock->mir_connection_create_screencast_sync(connection, parameters);
}

void mir_screencast_release_sync(MirScreencast *screencast) {
    global_mock->mir_screencast_release_sync(screencast);
}

MirBufferStream* mir_screencast_get_buffer_stream(MirScreencast *screencast) {
    return global_mock->mir_screencast_get_buffer_stream(screencast);
}

void mir_buffer_stream_get_current_buffer(MirBufferStream *buffer_stream,
    MirNativeBuffer **buffer_package) {
    global_mock->mir_buffer_stream_get_current_buffer(buffer_stream, buffer_package);
}

void mir_buffer_stream_swap_buffers_sync(MirBufferStream *buffer_stream) {
    global_mock->mir_buffer_stream_swap_buffers_sync(buffer_stream);
}
