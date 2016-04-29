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

#ifndef MCS_TEST_MIR_MOCKMIR_H_
#define MCS_TEST_MIR_MOCKMIR_H_

#include <gmock/gmock.h>

#include <mir_toolkit/mir_client_library.h>
#include <mir_toolkit/mir_screencast.h>
#include <mir_toolkit/mir_buffer_stream.h>

#include <memory>

namespace mcs {
namespace test {
namespace mir {

class MockMir {
public:
    typedef std::shared_ptr<MockMir> Ptr;

    MockMir();
    ~MockMir();

    MOCK_METHOD2(mir_connect_sync, MirConnection*(char const *server, char const *app_name));
    MOCK_METHOD1(mir_connection_is_valid, bool(MirConnection*));
    MOCK_METHOD1(mir_connection_release, void(MirConnection*));
    MOCK_METHOD1(mir_connection_get_error_message, char const*(MirConnection*));
    MOCK_METHOD1(mir_connection_create_display_config, MirDisplayConfiguration*(MirConnection*));
    MOCK_METHOD4(mir_connection_get_available_surface_formats, void(MirConnection*, MirPixelFormat*,
                                                                    unsigned const int, unsigned int*));

    MOCK_METHOD1(mir_create_screencast_spec, MirScreencastSpec*(MirConnection*));
    MOCK_METHOD2(mir_screencast_spec_set_width, void(MirScreencastSpec*, unsigned int));
    MOCK_METHOD2(mir_screencast_spec_set_height, void(MirScreencastSpec*, unsigned int));
    MOCK_METHOD2(mir_screencast_spec_set_pixel_format, void(MirScreencastSpec*, MirPixelFormat));
    MOCK_METHOD2(mir_screencast_spec_set_capture_region, void(MirScreencastSpec*, MirRectangle const*));
    MOCK_METHOD2(mir_screencast_spec_set_mirror_mode, void(MirScreencastSpec*, MirMirrorMode));
    MOCK_METHOD2(mir_screencast_spec_set_number_of_buffers, void(MirScreencastSpec*, unsigned int));
    MOCK_METHOD1(mir_screencast_spec_release, void(MirScreencastSpec*));
    MOCK_METHOD1(mir_screencast_create_sync, MirScreencast*(MirScreencastSpec*));
    MOCK_METHOD1(mir_screencast_is_valid, bool(MirScreencast*));
    MOCK_METHOD1(mir_screencast_get_error_message, char const*(MirScreencast*));
    MOCK_METHOD1(mir_screencast_release_sync, void(MirScreencast*));
    MOCK_METHOD1(mir_screencast_get_buffer_stream, MirBufferStream*(MirScreencast*));

    MOCK_METHOD2(mir_buffer_stream_get_current_buffer, void(MirBufferStream*, MirNativeBuffer**));
    MOCK_METHOD1(mir_buffer_stream_swap_buffers_sync, void(MirBufferStream*));
};

} // namespace mir
} // namespace tests
} // namespace mcs

#endif
