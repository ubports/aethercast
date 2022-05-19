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

#include <cstring>
#include <future>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "ac/logger.h"
#include "ac/mir/screencast.h"

namespace {
static constexpr const char *kMirSocket{"/run/mir_socket"};
static constexpr const char *kMirConnectionName{"aethercast screencast client"};
}

namespace ac {
namespace mir {

std::string mir_pixel_format_to_string(MirPixelFormat format)
{
    // Don't know of any big endian platform supported by mir
    switch(format)
    {
    case mir_pixel_format_abgr_8888:
        return "RGBA";
    case mir_pixel_format_xbgr_8888:
        return "RGBX";
    case mir_pixel_format_argb_8888:
        return "BGRA";
    case mir_pixel_format_xrgb_8888:
        return "BGRX";
    case mir_pixel_format_bgr_888:
        return "BGR";
    case mir_pixel_format_rgb_888:
        return "RGB";
    case mir_pixel_format_rgb_565:
        return "RGB565";
    case mir_pixel_format_rgba_5551:
        return "RGBA5551";
    case mir_pixel_format_rgba_4444:
        return "RGBA4444";
    default:
        throw std::logic_error("Invalid pixel format");
    }
}

class BufferStreamScreencast : public ScreencastFetcher
{
public:
    BufferStreamScreencast(ScreencastConfiguration* config,
                           MirBufferStream* buffer_stream,
                           bool readout)
        : ScreencastFetcher(readout),
          buffer_stream{buffer_stream},
          pixel_format_{mir_pixel_format_to_string(config->pixel_format)}
    {
        // Don't complete construction unless this is going to work later!
        MirGraphicsRegion const region{graphics_region_for(buffer_stream)};
        int const line_size{region.width * MIR_BYTES_PER_PIXEL(region.pixel_format)};

        buffer = video::Buffer::Create(line_size * region.height);
    }

    std::string pixel_format() override
    {
        return pixel_format_;
    }

    void Capture() override
    {
        if (readout_) {
            MirGraphicsRegion const region{graphics_region_for(buffer_stream)};
            int const line_size{region.width * MIR_BYTES_PER_PIXEL(region.pixel_format)};
            // Contents are rendered up-side down, read them bottom to top
            auto addr = region.vaddr + (region.height - 1)*region.stride;
            auto write_addr = buffer->Data();
            for (int i = 0; i < region.height; i++)
            {
                std::memcpy(write_addr, addr, line_size);
                addr -= region.stride;
                write_addr += region.stride;
            }
        }
        mir_buffer_stream_swap_buffers_sync(buffer_stream);
    }

    video::Buffer::Ptr CurrentReadout() override {
        return buffer;
    }

private:
    MirGraphicsRegion graphics_region_for(MirBufferStream* buffer_stream)
    {
        MirGraphicsRegion region{0, 0, 0, mir_pixel_format_invalid, nullptr};
        mir_buffer_stream_get_graphics_region(buffer_stream, &region);

        if (region.vaddr == nullptr)
            throw std::runtime_error("Failed to obtain screencast buffer");

        return region;
    }

    MirBufferStream* const buffer_stream;
    std::string const pixel_format_;
    video::Buffer::Ptr buffer;
};

class EGLScreencast : public ScreencastFetcher
{
public:
    EGLScreencast(MirConnection* connection, ScreencastConfiguration* config,
                  MirBufferStream* buffer_stream, bool readout)
        : ScreencastFetcher(readout),
          x{0},
          y{0},
          width{config->width},
          height{config->height}
    {
        static EGLint const attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE};

        static EGLint const context_attribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE };
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

            AC_DEBUG("1");
        auto native_display =
            reinterpret_cast<EGLNativeDisplayType>(
                mir_connection_get_egl_native_display(connection));

            AC_DEBUG("2");

        auto native_window =
            reinterpret_cast<EGLNativeWindowType>(
                mir_buffer_stream_get_egl_native_window(buffer_stream));

        AC_DEBUG("3");

#pragma GCC diagnostic pop
        egl_display = eglGetDisplay(native_display);

        AC_DEBUG("4");

        eglInitialize(egl_display, nullptr, nullptr);

        AC_DEBUG("5");

        int n;
        eglChooseConfig(egl_display, attribs, &egl_config, 1, &n);

        AC_DEBUG("6");

        egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, NULL);
        if (egl_surface == EGL_NO_SURFACE)
            throw std::runtime_error("Failed to create EGL screencast surface");

        AC_DEBUG("7");

        egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
        if (egl_context == EGL_NO_CONTEXT)
            throw std::runtime_error("Failed to create EGL context for screencast");

        AC_DEBUG("8");

        if (eglMakeCurrent(egl_display, egl_surface,
                           egl_surface, egl_context) != EGL_TRUE)
        {
            throw std::runtime_error("Failed to make screencast surface current");
        }

        AC_DEBUG("9");

        if (eglSwapBuffers(egl_display, egl_surface) != EGL_TRUE)
            AC_WARNING("Failed to swap screencast surface buffers");

        AC_DEBUG("10");

        uint32_t a_pixel;
        glReadPixels(0, 0, 1, 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, &a_pixel);
        if (glGetError() == GL_NO_ERROR)
            read_pixel_format = GL_BGRA_EXT;
        else
            read_pixel_format = GL_RGBA;

        int const rgba_pixel_size{4};
        auto const frame_size_bytes = rgba_pixel_size * width * height;
        buffer = video::Buffer::Create(frame_size_bytes);
    }

    ~EGLScreencast()
    {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(egl_display, egl_surface);
        eglDestroyContext(egl_display, egl_context);
        eglTerminate(egl_display);
    }

    void Capture() override
    {
        if (eglMakeCurrent(egl_display, egl_surface,
                           egl_surface, egl_context) != EGL_TRUE)
        {
            AC_WARNING("Failed to make screencast surface current");
        }

        if (readout_) {
            void* data = buffer->Data();
            glReadPixels(x, y, width, height, read_pixel_format, GL_UNSIGNED_BYTE, data);
        }

        if (eglSwapBuffers(egl_display, egl_surface) != EGL_TRUE)
            AC_WARNING("Failed to swap screencast surface buffers");

        if (eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
        {
            AC_WARNING("Failed to make screencast surface current");
        }
    }

    video::Buffer::Ptr CurrentReadout() override {
        return buffer;
    }

    std::string pixel_format() override
    {
        return read_pixel_format == GL_BGRA_EXT ? "BGRA" : "RGBA";
    }    

private:
    unsigned int const x;
    unsigned int const y;
    unsigned int const width;
    unsigned int const height;
    video::Buffer::Ptr buffer;
    EGLDisplay egl_display;
    EGLContext egl_context;
    EGLSurface egl_surface;
    EGLConfig egl_config;
    GLenum read_pixel_format;
};

#if 1
std::unique_ptr<ScreencastFetcher> CreateFetcher(MirConnection* connection,
                                                 ScreencastConfiguration* config,
                                                 MirBufferStream* buffer_stream,
                                                 bool readout)
{
    try
    {
        return std::make_unique<BufferStreamScreencast>(config, buffer_stream, readout);
    }
    catch(...)
    {
    }
    // Fallback to EGL if MirBufferStream can't be used directly
    return std::make_unique<EGLScreencast>(connection, config, buffer_stream, readout);
}
#else
std::unique_ptr<ScreencastFetcher> CreateFetcher(MirConnection* connection,
                                                 ScreencastConfiguration* config,
                                                 MirBufferStream* buffer_stream,
                                                 bool readout)
{
    //return std::make_unique<BufferStreamScreencast>(config, buffer_stream, readout);
    // Fallback to EGL if MirBufferStream can't be used directly
    return std::make_unique<EGLScreencast>(connection, config, buffer_stream, readout);
}
#endif

Screencast::Screencast(bool readout) :
    connection_(nullptr),
    screencast_(nullptr),
    buffer_stream_(nullptr),
    readout_(readout) {
}

Screencast::~Screencast() {
    AC_DEBUG("");

    if (screencast_)
        mir_screencast_release_sync(screencast_);

    if (connection_)
        mir_connection_release(connection_);
}

bool Screencast::Setup(const video::DisplayOutput &output) {
    if (screencast_ || connection_ || buffer_stream_)
        return false;

    if (output.mode != video::DisplayOutput::Mode::kExtend) {
        AC_ERROR("Unsupported display output mode specified '%s'", output.mode);
        return false;
    }

    AC_DEBUG("Setting up screencast [%s %dx%d]", output.mode,
              output.width, output.height);

    connection_ = mir_connect_sync(kMirSocket, kMirConnectionName);
    if (!mir_connection_is_valid(connection_)) {
        AC_ERROR("Failed to connect to Mir server: %s",
                  mir_connection_get_error_message(connection_));
        return false;
    }

    const auto config = mir_connection_create_display_config(connection_);
    if (!config) {
        AC_ERROR("Failed to create display configuration: %s",
                  mir_connection_get_error_message(connection_));
        return false;
    }

    MirDisplayOutput *active_output = nullptr;
    unsigned int output_index = 0;
    unsigned int virtual_index = 0;

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

    for (unsigned int i = 0; i < config->num_outputs; ++i) {
        if (config->outputs[i].type == MirDisplayOutputType::mir_display_output_type_virtual) {
            virtual_index = i;
            break;
        }
    }

    if (!active_output) {
        AC_ERROR("Failed to find a suitable display output");
        return false;
    }

    const MirDisplayMode *display_mode = &active_output->modes[active_output->current_mode];

    MirRectangle region;
    // If we request a screen region outside the available screen area
    // mir will create a mir output which is then available for everyone
    // as just another display.
    region.width = output.width;
    region.height = output.height;
    region.left = display_mode->horizontal_resolution;
    region.top = 0;

    AC_INFO("Selected output ID %i [(%ix%i)+(%ix%i)] orientation %d",
             output_index,
             region.width,
             region.height,
             region.left, region.top,
             active_output->orientation);

    unsigned int num_pixel_formats = 0;
    MirPixelFormat pixel_format;
    mir_connection_get_available_surface_formats(connection_, &pixel_format,
                                                 1, &num_pixel_formats);
    if (num_pixel_formats == 0) {
        AC_ERROR("Failed to find suitable pixel format: %s",
                  mir_connection_get_error_message(connection_));
        return false;
    }

    auto spec = mir_create_screencast_spec(connection_);
    if (!spec) {
        AC_ERROR("Failed to create Mir screencast specification: %s",
              mir_screencast_get_error_message(screencast_));
        return false;
    }

    mir_screencast_spec_set_capture_region(spec, &region);
    mir_screencast_spec_set_width(spec, output.width);
    mir_screencast_spec_set_height(spec, output.height);
    mir_screencast_spec_set_pixel_format(spec, pixel_format);
    mir_screencast_spec_set_mirror_mode(spec, readout_ ? mir_mirror_mode_none : mir_mirror_mode_vertical);
    mir_screencast_spec_set_number_of_buffers(spec, 2);

    screencast_ = mir_screencast_create_sync(spec);
    mir_screencast_spec_release(spec);
    if (!mir_screencast_is_valid(screencast_)) {
        AC_ERROR("Failed to create Mir screencast: %s",
              mir_screencast_get_error_message(screencast_));
        return false;
    }

    buffer_stream_ = mir_screencast_get_buffer_stream(screencast_);
    if (!buffer_stream_) {
        AC_ERROR("Failed to setup Mir buffer stream");
        return false;
    }

    output_ = output;
    output_.refresh_rate = display_mode->refresh_rate;

    fetcher_config_.width = region.width;
    fetcher_config_.height = region.height;
    fetcher_config_.pixel_format = pixel_format;

    return true;
}

void Screencast::SwapBuffers() {
    if (!buffer_stream_)
        return;

    if (!fetcher_)
        fetcher_ = CreateFetcher(connection_, &fetcher_config_, buffer_stream_, readout_);

    fetcher_->Capture();
}

video::DisplayOutput Screencast::OutputMode() const {
    return output_;
}

video::Buffer::Ptr Screencast::CurrentBuffer() const {
    if (!buffer_stream_)
        return ac::video::Buffer::Create((uint32_t)0);

    if (readout_) {
        return fetcher_->CurrentReadout();
    } else {
        MirNativeBuffer *buffer = nullptr;
        mir_buffer_stream_get_current_buffer(buffer_stream_, &buffer);
        auto ret = reinterpret_cast<void*>(buffer);
        return ac::video::Buffer::Create(ret);
    }
}
} // namespace mir
} // namespace ac
