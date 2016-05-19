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

#include "ac/video/utils.h"

namespace from_android {
extern bool IsIDR(const ac::video::Buffer::Ptr &buffer);
extern bool GetNextNALUnit(const uint8_t **_data, size_t *_size, const uint8_t **nalStart,
                           size_t *nalSize, bool startCodeFollows);
}

namespace ac {
namespace video {

bool DoesBufferContainIDRFrame(const ac::video::Buffer::Ptr &buffer) {
    return from_android::IsIDR(buffer);
}

bool GetNextNALUnit(const uint8_t **_data, size_t *_size, const uint8_t **nalStart,
                    size_t *nalSize, bool startCodeFollows) {
    return from_android::GetNextNALUnit(_data, _size, nalStart, nalSize, startCodeFollows);
}

} // video
} // mcs
