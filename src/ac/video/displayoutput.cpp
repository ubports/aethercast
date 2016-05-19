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

#include <iostream>

#include "ac/video/displayoutput.h"

namespace ac {
namespace video {

std::ostream& operator<<(std::ostream& out, DisplayOutput::Mode mode) {
    switch (mode) {
    case DisplayOutput::Mode::kExtend:
        return out << "extend";
    case DisplayOutput::Mode::kMirror:
        return out << "mirror";
    default:
        break;
    }
    return out << "unknown";
}

DisplayOutput::DisplayOutput() :
    mode(Mode::kMirror),
    width(0),
    height(0),
    refresh_rate(0) {
}

DisplayOutput::DisplayOutput(const DisplayOutput::Mode &mode, const unsigned int &width,
                             const unsigned int &height, const double &refresh_rate) :
    mode(mode),
    width(width),
    height(height),
    refresh_rate(refresh_rate) {
}

} // namespace video
} // namespace ac
