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

#ifndef AC_VIDEO_SENDERREPORT_H_
#define AC_VIDEO_SENDERREPORT_H_

#include <memory>

#include "ac/non_copyable.h"

#include "ac/utils.h"

namespace ac {
namespace video {

class SenderReport : public ac::NonCopyable {
public:
    typedef std::shared_ptr<SenderReport> Ptr;

    virtual void SentPacket(const ac::TimestampUs &timestamp, const size_t &size) = 0;
};

} // namespace video
} // namespace ac

#endif
