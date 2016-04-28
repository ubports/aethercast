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

#ifndef MCS_MIR_CONNECTOR_H_
#define MCS_MIR_CONNECTOR_H_

#include <memory>

#include <mir_toolkit/mir_client_library.h>
#include <mir_toolkit/mir_screencast.h>
#include <mir_toolkit/mir_buffer_stream.h>

#include "mcs/non_copyable.h"

#include "mcs/video/bufferproducer.h"

namespace mcs {
namespace mir {

class Screencast : public mcs::video::BufferProducer {
public:
    explicit Screencast();
    ~Screencast();

    bool Setup(const video::DisplayOutput &output) override;

    // From mcs::video::BufferProducer
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
} // namespace mcs

#endif
