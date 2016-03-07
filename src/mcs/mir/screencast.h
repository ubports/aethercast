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

#ifndef MCS_MIR_CONNECTOR_H_
#define MCS_MIR_CONNECTOR_H_

#include <memory>

#include <mir_toolkit/mir_client_library.h>
#include <mir_toolkit/mir_screencast.h>
#include <mir_toolkit/mir_buffer_stream.h>

#include "mcs/non_copyable.h"

namespace mcs {
namespace mir {

class Screencast : public mcs::NonCopyable {
public:
    typedef std::shared_ptr<Screencast> Ptr;

    enum class DisplayMode {
        kMirror,
        kExtend
    };

    static std::string DisplayModeToString(const DisplayMode &mode);

    struct DisplayOutput {
        DisplayMode mode;
        unsigned int width;
        unsigned int height;
        double refresh_rate;
    };

    explicit Screencast(const DisplayOutput &output);
    ~Screencast();

    void SwapBuffers();
    void SwapBuffersSync();

    bool IsValid() const;

    void* NativeWindowHandle() const;
    void* NativeDisplayHandle() const;

    DisplayOutput OutputMode() const;
    MirNativeBuffer* CurrentBuffer() const;


private:
    MirConnection *connection_;
    MirScreencast *screencast_;
    MirBufferStream *buffer_stream_;
    MirScreencastParameters params_;
    DisplayOutput output_;
};

} // namespace mir
} // namespace mcs

#endif
