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

#ifndef AC_VIDEO_DISPLAYOUTPUT_H_
#define AC_VIDEO_DISPLAYOUTPUT_H_

#include <string>

namespace ac {
namespace video {

struct DisplayOutput {
    enum class Mode {
        kMirror,
        kExtend
    };

    DisplayOutput();
    DisplayOutput(const DisplayOutput::Mode &mode, const unsigned int &width,
                  const unsigned int &height, const double &refresh_rate);

    static std::string ModeToString(const Mode &mode);

    Mode mode;
    unsigned int width;
    unsigned int height;
    double refresh_rate;
};

std::ostream& operator<<(std::ostream& out, DisplayOutput::Mode mode);

} // namespace video
} // namespace ac

#endif
