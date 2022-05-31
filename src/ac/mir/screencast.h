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

class Screencast : public ac::video::BufferProducer {
public:
    explicit Screencast();
    ~Screencast();

    bool Setup(const video::DisplayOutput &output) override;
    bool Stop() override;

    // From ac::video::BufferProducer
    void SwapBuffers() override;
    void* CurrentBuffer() const override;
    video::DisplayOutput OutputMode() const override;

private:
    MirConnection *connection_;
    MirScreencast *screencast_;
    MirBufferStream *buffer_stream_;
    video::DisplayOutput output_;
};

} // namespace mir
} // namespace ac

#endif
