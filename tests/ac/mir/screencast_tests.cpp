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

#include <boost/concept_check.hpp>

#include <gmock/gmock.h>

#include "mockmir.h"

#include "ac/mir/screencast.h"

using namespace ::testing;

namespace {
struct TestMirScreencastSpec {
};
}

TEST(Screencast, DoesNotSupportMirrorMode) {
    ac::video::DisplayOutput output;
    ::memset(&output, 0, sizeof(output));
    output.mode = ac::video::DisplayOutput::Mode::kMirror;

    const auto screencast = std::make_shared<ac::mir::Screencast>();
    EXPECT_FALSE(screencast->Setup(output));
}

TEST(Screencast, ConnectToMirFailsCorrectly) {
    auto mir = std::make_shared<ac::test::mir::MockMir>();

    EXPECT_CALL(*mir, mir_connect_sync(_, _))
            .Times(1)
            .WillOnce(Return(nullptr));

    EXPECT_CALL(*mir, mir_connection_is_valid(nullptr))
            .Times(1)
            .WillOnce(Return(false));

    EXPECT_CALL(*mir, mir_connection_get_error_message(nullptr))
            .Times(1)
            .WillOnce(Return("Error message from mock"));

    ac::video::DisplayOutput output;
    output.mode = ac::video::DisplayOutput::Mode::kExtend;
    const auto screencast = std::make_shared<ac::mir::Screencast>();

    EXPECT_FALSE(screencast->Setup(output));
}

TEST(Screencast, DoesNotCrashOnInvalidDisplayConfig) {
    auto mir = std::make_shared<ac::test::mir::MockMir>();

    auto connection = reinterpret_cast<MirConnection*>(1);

    EXPECT_CALL(*mir, mir_connect_sync(_, _))
            .Times(1)
            .WillOnce(Return(connection));

    EXPECT_CALL(*mir, mir_connection_release(connection))
            .Times(1);

    EXPECT_CALL(*mir, mir_connection_is_valid(connection))
            .Times(1)
            .WillOnce(Return(true));

    EXPECT_CALL(*mir, mir_connection_create_display_config(connection))
            .Times(1)
            .WillOnce(Return(nullptr));

    EXPECT_CALL(*mir, mir_connection_get_error_message(connection))
            .Times(1)
            .WillOnce(Return("Error message from mock"));

    ac::video::DisplayOutput output;
    output.mode = ac::video::DisplayOutput::Mode::kExtend;
    const auto screencast = std::make_shared<ac::mir::Screencast>();

    EXPECT_FALSE(screencast->Setup(output));
}

TEST(Screencast, NoDisplayConfigurationAvailable) {
    auto mir = std::make_shared<ac::test::mir::MockMir>();

    auto connection = reinterpret_cast<MirConnection*>(1);

    EXPECT_CALL(*mir, mir_connect_sync(_, _))
            .Times(1)
            .WillRepeatedly(Return(connection));

    EXPECT_CALL(*mir, mir_connection_release(connection))
            .Times(1);

    EXPECT_CALL(*mir, mir_connection_is_valid(connection))
            .Times(1)
            .WillRepeatedly(Return(true));

    MirDisplayConfiguration display_config;
    ::memset(&display_config, 0, sizeof(display_config));

    EXPECT_CALL(*mir, mir_connection_create_display_config(connection))
            .Times(1)
            .WillRepeatedly(Return(&display_config));

    ac::video::DisplayOutput output;
    output.mode = ac::video::DisplayOutput::Mode::kExtend;
    const auto screencast = std::make_shared<ac::mir::Screencast>();
    EXPECT_FALSE(screencast->Setup(output));
}

TEST(Screencast, NoUsableDisplayConfigurationAvailable) {
    auto mir = std::make_shared<ac::test::mir::MockMir>();

    auto connection = reinterpret_cast<MirConnection*>(1);

    EXPECT_CALL(*mir, mir_connect_sync(_, _))
            .Times(1)
            .WillRepeatedly(Return(connection));

    EXPECT_CALL(*mir, mir_connection_release(connection))
            .Times(1);

    EXPECT_CALL(*mir, mir_connection_is_valid(connection))
            .Times(1)
            .WillRepeatedly(Return(true));

    MirDisplayConfiguration display_config;
    display_config.num_outputs = 2;
    display_config.outputs = new MirDisplayOutput[2];

    display_config.outputs[0].connected = false;
    display_config.outputs[0].used = false;
    display_config.outputs[0].current_mode = 0;
    display_config.outputs[0].num_modes = 0;

    display_config.outputs[1].connected = false;
    display_config.outputs[1].used = false;
    display_config.outputs[1].current_mode = 0;
    display_config.outputs[1].num_modes = 0;

    EXPECT_CALL(*mir, mir_connection_create_display_config(connection))
            .Times(1)
            .WillRepeatedly(Return(&display_config));

    ac::video::DisplayOutput output;
    output.mode = ac::video::DisplayOutput::Mode::kExtend;
    const auto screencast = std::make_shared<ac::mir::Screencast>();
    EXPECT_FALSE(screencast->Setup(output));
}


TEST(Screencast, NoPixelFormatAvailable) {
    auto mir = std::make_shared<ac::test::mir::MockMir>();

    ac::video::DisplayOutput output;
    output.mode = ac::video::DisplayOutput::Mode::kExtend;
    output.width = 1280;
    output.height = 720;

    auto connection = reinterpret_cast<MirConnection*>(1);

    EXPECT_CALL(*mir, mir_connect_sync(_, _))
            .Times(1)
            .WillRepeatedly(Return(connection));

    EXPECT_CALL(*mir, mir_connection_release(connection))
            .Times(1);

    EXPECT_CALL(*mir, mir_connection_is_valid(connection))
            .Times(1)
            .WillRepeatedly(Return(true));

    MirDisplayConfiguration display_config;
    display_config.num_outputs = 1;
    display_config.outputs = new MirDisplayOutput[1];

    display_config.outputs[0].connected = true;
    display_config.outputs[0].used = true;
    display_config.outputs[0].current_mode = 0;
    display_config.outputs[0].num_modes = 1;
    display_config.outputs[0].modes = new MirDisplayMode[1];
    display_config.outputs[0].modes[0].horizontal_resolution = 1280;
    display_config.outputs[0].modes[0].vertical_resolution = 720;
    display_config.outputs[0].modes[0].refresh_rate = 30;

    EXPECT_CALL(*mir, mir_connection_create_display_config(connection))
            .Times(1)
            .WillRepeatedly(Return(&display_config));

    TestMirScreencastSpec spec;
    auto mir_spec = reinterpret_cast<MirScreencastSpec*>(&spec);

    EXPECT_CALL(*mir, mir_create_screencast_spec(connection))
            .Times(1)
            .WillRepeatedly(Return(mir_spec));

    EXPECT_CALL(*mir, mir_screencast_spec_set_width(mir_spec, output.width))
            .Times(1);
    EXPECT_CALL(*mir, mir_screencast_spec_set_height(mir_spec, output.height))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_spec_set_capture_region(mir_spec, _))
            .Times(1)
            .WillRepeatedly(Invoke([&](MirScreencastSpec *spec, MirRectangle const *rect) {
                boost::ignore_unused_variable_warning(spec);
                EXPECT_EQ(display_config.outputs[0].modes[0].horizontal_resolution, rect->left);
                EXPECT_EQ(0, rect->top);
                EXPECT_EQ(display_config.outputs[0].modes[0].vertical_resolution, output.height);
                EXPECT_EQ(display_config.outputs[0].modes[0].horizontal_resolution, output.width);
            }));

    EXPECT_CALL(*mir, mir_connection_get_available_surface_formats(connection, _, _, _))
            .Times(1);

    EXPECT_CALL(*mir, mir_connection_get_error_message(connection))
            .Times(1)
            .WillOnce(Return("Error message from mock"));

    const auto screencast = std::make_shared<ac::mir::Screencast>();
    EXPECT_FALSE(screencast->Setup(output));
}

TEST(Screencast, ScreencastCreationFails) {
    auto mir = std::make_shared<ac::test::mir::MockMir>();

    ac::video::DisplayOutput output;
    output.mode = ac::video::DisplayOutput::Mode::kExtend;
    output.width = 1280;
    output.height = 720;

    auto connection = reinterpret_cast<MirConnection*>(1);

    EXPECT_CALL(*mir, mir_connect_sync(_, _))
            .Times(1)
            .WillRepeatedly(Return(connection));

    EXPECT_CALL(*mir, mir_connection_release(connection))
            .Times(1);

    EXPECT_CALL(*mir, mir_connection_is_valid(connection))
            .Times(1)
            .WillRepeatedly(Return(true));

    MirDisplayConfiguration display_config;
    display_config.num_outputs = 1;
    display_config.outputs = new MirDisplayOutput[1];

    display_config.outputs[0].connected = true;
    display_config.outputs[0].used = true;
    display_config.outputs[0].current_mode = 0;
    display_config.outputs[0].num_modes = 1;
    display_config.outputs[0].modes = new MirDisplayMode[1];
    display_config.outputs[0].modes[0].horizontal_resolution = 1280;
    display_config.outputs[0].modes[0].vertical_resolution = 720;
    display_config.outputs[0].modes[0].refresh_rate = 30;

    EXPECT_CALL(*mir, mir_connection_create_display_config(connection))
            .Times(1)
            .WillRepeatedly(Return(&display_config));

    TestMirScreencastSpec spec;
    auto mir_spec = reinterpret_cast<MirScreencastSpec*>(&spec);

    EXPECT_CALL(*mir, mir_create_screencast_spec(connection))
            .Times(1)
            .WillRepeatedly(Return(mir_spec));

    EXPECT_CALL(*mir, mir_screencast_spec_set_width(mir_spec, output.width))
            .Times(1);
    EXPECT_CALL(*mir, mir_screencast_spec_set_height(mir_spec, output.height))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_spec_set_capture_region(mir_spec, _))
            .Times(1)
            .WillRepeatedly(Invoke([&](MirScreencastSpec *spec, MirRectangle const *rect) {
                boost::ignore_unused_variable_warning(spec);
                EXPECT_EQ(display_config.outputs[0].modes[0].horizontal_resolution, rect->left);
                EXPECT_EQ(0, rect->top);
                EXPECT_EQ(display_config.outputs[0].modes[0].vertical_resolution, output.height);
                EXPECT_EQ(display_config.outputs[0].modes[0].horizontal_resolution, output.width);
            }));

    EXPECT_CALL(*mir, mir_connection_get_available_surface_formats(connection, _, _, _))
            .Times(1)
            .WillOnce(SetArgPointee<3>(mir_pixel_format_abgr_8888));

    EXPECT_CALL(*mir, mir_screencast_spec_set_pixel_format(mir_spec, _))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_spec_set_mirror_mode(mir_spec, mir_mirror_mode_vertical))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_spec_set_number_of_buffers(mir_spec, 1))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_create_sync(mir_spec))
            .Times(1)
            .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(*mir, mir_screencast_spec_release(mir_spec))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_is_valid(nullptr))
            .Times(1)
            .WillRepeatedly(Return(false));

    EXPECT_CALL(*mir, mir_screencast_get_error_message(nullptr))
            .Times(1)
            .WillOnce(Return("Error message from mock"));

    const auto screencast = std::make_shared<ac::mir::Screencast>();
    EXPECT_FALSE(screencast->Setup(output));
}

TEST(Screencast, ScreencastDoesNotProvideBufferStream) {
    auto mir = std::make_shared<ac::test::mir::MockMir>();

    ac::video::DisplayOutput output;
    output.mode = ac::video::DisplayOutput::Mode::kExtend;
    output.width = 1280;
    output.height = 720;

    auto connection = reinterpret_cast<MirConnection*>(1);

    EXPECT_CALL(*mir, mir_connect_sync(_, _))
            .Times(1)
            .WillRepeatedly(Return(connection));

    EXPECT_CALL(*mir, mir_connection_release(connection))
            .Times(1);

    EXPECT_CALL(*mir, mir_connection_is_valid(connection))
            .Times(1)
            .WillRepeatedly(Return(true));

    MirDisplayConfiguration display_config;
    display_config.num_outputs = 1;
    display_config.outputs = new MirDisplayOutput[1];

    display_config.outputs[0].connected = true;
    display_config.outputs[0].used = true;
    display_config.outputs[0].current_mode = 0;
    display_config.outputs[0].num_modes = 1;
    display_config.outputs[0].modes = new MirDisplayMode[1];
    display_config.outputs[0].modes[0].horizontal_resolution = 1280;
    display_config.outputs[0].modes[0].vertical_resolution = 720;
    display_config.outputs[0].modes[0].refresh_rate = 30;

    EXPECT_CALL(*mir, mir_connection_create_display_config(connection))
            .Times(1)
            .WillRepeatedly(Return(&display_config));

    TestMirScreencastSpec spec;
    auto mir_spec = reinterpret_cast<MirScreencastSpec*>(&spec);

    EXPECT_CALL(*mir, mir_create_screencast_spec(connection))
            .Times(1)
            .WillRepeatedly(Return(mir_spec));

    EXPECT_CALL(*mir, mir_screencast_spec_set_width(mir_spec, output.width))
            .Times(1);
    EXPECT_CALL(*mir, mir_screencast_spec_set_height(mir_spec, output.height))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_spec_set_capture_region(mir_spec, _))
            .Times(1)
            .WillRepeatedly(Invoke([&](MirScreencastSpec *spec, MirRectangle const *rect) {
                boost::ignore_unused_variable_warning(spec);
                EXPECT_EQ(display_config.outputs[0].modes[0].horizontal_resolution, rect->left);
                EXPECT_EQ(0, rect->top);
                EXPECT_EQ(display_config.outputs[0].modes[0].vertical_resolution, output.height);
                EXPECT_EQ(display_config.outputs[0].modes[0].horizontal_resolution, output.width);
            }));

    EXPECT_CALL(*mir, mir_connection_get_available_surface_formats(connection, _, _, _))
            .Times(1)
            .WillOnce(SetArgPointee<3>(mir_pixel_format_abgr_8888));

    EXPECT_CALL(*mir, mir_screencast_spec_set_pixel_format(mir_spec, _))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_spec_set_mirror_mode(mir_spec, mir_mirror_mode_vertical))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_spec_set_number_of_buffers(mir_spec, 1))
            .Times(1);

    auto mir_screencast = reinterpret_cast<MirScreencast*>(2);

    EXPECT_CALL(*mir, mir_screencast_create_sync(mir_spec))
            .Times(1)
            .WillRepeatedly(Return(mir_screencast));

    EXPECT_CALL(*mir, mir_screencast_spec_release(mir_spec))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_is_valid(mir_screencast))
            .Times(1)
            .WillRepeatedly(Return(true));

    EXPECT_CALL(*mir, mir_screencast_get_buffer_stream(mir_screencast))
            .Times(1)
            .WillOnce(Return(nullptr));

    EXPECT_CALL(*mir, mir_buffer_stream_swap_buffers_sync(_))
            .Times(0);

    EXPECT_CALL(*mir, mir_screencast_release_sync(mir_screencast))
            .Times(1);

    const auto screencast = std::make_shared<ac::mir::Screencast>();
    EXPECT_FALSE(screencast->Setup(output));

    // Also make sure that swap buffers isn't called when we don't have
    // a buffer stream and the screencast doesn't return a valid buffer
    screencast->SwapBuffers();
    EXPECT_EQ(nullptr, screencast->CurrentBuffer());
}


TEST(Screencast, DoesSwapBuffersAndReturnsCurrentBuffer) {
    auto mir = std::make_shared<ac::test::mir::MockMir>();

    ac::video::DisplayOutput output;
    output.mode = ac::video::DisplayOutput::Mode::kExtend;
    output.width = 1280;
    output.height = 720;
    output.refresh_rate = 30;

    auto connection = reinterpret_cast<MirConnection*>(1);

    EXPECT_CALL(*mir, mir_connect_sync(_, _))
            .Times(1)
            .WillRepeatedly(Return(connection));

    EXPECT_CALL(*mir, mir_connection_release(connection))
            .Times(1);

    EXPECT_CALL(*mir, mir_connection_is_valid(connection))
            .Times(1)
            .WillRepeatedly(Return(true));

    MirDisplayConfiguration display_config;
    display_config.num_outputs = 1;
    display_config.outputs = new MirDisplayOutput[1];

    display_config.outputs[0].connected = true;
    display_config.outputs[0].used = true;
    display_config.outputs[0].current_mode = 0;
    display_config.outputs[0].num_modes = 1;
    display_config.outputs[0].modes = new MirDisplayMode[1];
    display_config.outputs[0].modes[0].horizontal_resolution = 1280;
    display_config.outputs[0].modes[0].vertical_resolution = 720;
    display_config.outputs[0].modes[0].refresh_rate = 30;

    EXPECT_CALL(*mir, mir_connection_create_display_config(connection))
            .Times(1)
            .WillRepeatedly(Return(&display_config));

    TestMirScreencastSpec spec;
    auto mir_spec = reinterpret_cast<MirScreencastSpec*>(&spec);

    EXPECT_CALL(*mir, mir_create_screencast_spec(connection))
            .Times(1)
            .WillRepeatedly(Return(mir_spec));

    EXPECT_CALL(*mir, mir_screencast_spec_set_width(mir_spec, output.width))
            .Times(1);
    EXPECT_CALL(*mir, mir_screencast_spec_set_height(mir_spec, output.height))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_spec_set_capture_region(mir_spec, _))
            .Times(1)
            .WillRepeatedly(Invoke([&](MirScreencastSpec *spec, MirRectangle const *rect) {
                boost::ignore_unused_variable_warning(spec);
                EXPECT_EQ(display_config.outputs[0].modes[0].horizontal_resolution, rect->left);
                EXPECT_EQ(0, rect->top);
                EXPECT_EQ(display_config.outputs[0].modes[0].vertical_resolution, output.height);
                EXPECT_EQ(display_config.outputs[0].modes[0].horizontal_resolution, output.width);
            }));

    EXPECT_CALL(*mir, mir_connection_get_available_surface_formats(connection, _, _, _))
            .Times(1)
            .WillOnce(SetArgPointee<3>(mir_pixel_format_abgr_8888));

    EXPECT_CALL(*mir, mir_screencast_spec_set_pixel_format(mir_spec, _))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_spec_set_mirror_mode(mir_spec, mir_mirror_mode_vertical))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_spec_set_number_of_buffers(mir_spec, 1))
            .Times(1);

    auto mir_screencast = reinterpret_cast<MirScreencast*>(2);

    EXPECT_CALL(*mir, mir_screencast_create_sync(mir_spec))
            .Times(1)
            .WillRepeatedly(Return(mir_screencast));

    EXPECT_CALL(*mir, mir_screencast_spec_release(mir_spec))
            .Times(1);

    EXPECT_CALL(*mir, mir_screencast_is_valid(mir_screencast))
            .Times(1)
            .WillRepeatedly(Return(true));

    auto buffer_stream = reinterpret_cast<MirBufferStream*>(3);

    EXPECT_CALL(*mir, mir_screencast_get_buffer_stream(mir_screencast))
            .Times(1)
            .WillOnce(Return(buffer_stream));

    EXPECT_CALL(*mir, mir_buffer_stream_swap_buffers_sync(buffer_stream))
            .Times(1);

    auto expected_buffer = reinterpret_cast<MirNativeBuffer*>(4);

    EXPECT_CALL(*mir, mir_buffer_stream_get_current_buffer(buffer_stream, _))
            .Times(1)
            .WillOnce(SetArgPointee<1>(expected_buffer));

    EXPECT_CALL(*mir, mir_screencast_release_sync(mir_screencast))
            .Times(1);

    const auto screencast = std::make_shared<ac::mir::Screencast>();
    EXPECT_TRUE(screencast->Setup(output));

    auto returned_output = screencast->OutputMode();
    EXPECT_EQ(output.width, returned_output.width);
    EXPECT_EQ(output.height, returned_output.height);
    EXPECT_EQ(output.refresh_rate, returned_output.refresh_rate);
    EXPECT_EQ(output.mode, returned_output.mode);

    screencast->SwapBuffers();
    EXPECT_EQ(expected_buffer, screencast->CurrentBuffer());
}
