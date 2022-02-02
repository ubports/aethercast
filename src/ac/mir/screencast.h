/*
 * Copyright (C) 2015-2016 Canonical, Ltd.
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

#ifndef AC_MIR_CONNECTOR_H_
#define AC_MIR_CONNECTOR_H_

#include <memory>

#include <mir_toolkit/mir_client_library.h>
#include <mir_toolkit/mir_screencast.h>
#include <mir_toolkit/mir_buffer_stream.h>

#include "ac/non_copyable.h"

#include "ac/video/bufferproducer.h"

namespace ac {
namespace mir {

struct ScreencastConfiguration
{
    uint32_t width;
    uint32_t height;
    MirPixelFormat pixel_format;
};

class ScreencastFetcher
{
public:
    virtual ~ScreencastFetcher() = default;
    virtual std::string pixel_format() = 0;

    virtual void Capture() = 0;
    virtual ac::video::Buffer::Ptr CurrentReadout() = 0;

protected:
    ScreencastFetcher(bool readout) :
        readout_(readout)
    {
    }

    bool readout_;

private:
    ScreencastFetcher(ScreencastFetcher const&) = delete;
    ScreencastFetcher& operator=(ScreencastFetcher const&) = delete;
};

class Screencast : public ac::video::BufferProducer {
public:
    explicit Screencast(bool readout = false);
    ~Screencast();

    bool Setup(const video::DisplayOutput &output) override;

    // From ac::video::BufferProducer
    void SwapBuffers() override;
    video::Buffer::Ptr CurrentBuffer() const override;
    video::DisplayOutput OutputMode() const override;

private:
    MirConnection *connection_;
    MirScreencast *screencast_;
    ScreencastConfiguration fetcher_config_;
    std::unique_ptr<ScreencastFetcher> fetcher_;
    MirBufferStream *buffer_stream_;
    video::DisplayOutput output_;
    bool readout_;
};

} // namespace mir
} // namespace ac

#endif
