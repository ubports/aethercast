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

#ifndef MCS_VIDEO_ENCODERREPORT_H_
#define MCS_VIDEO_ENCODERREPORT_H_

#include <memory>

#include "mcs/non_copyable.h"

#include "mcs/utils.h"

namespace mcs {
namespace video {

class EncoderReport : public mcs::NonCopyable {
public:
    typedef std::shared_ptr<EncoderReport> Ptr;

    virtual void Started() = 0;
    virtual void Stopped() = 0;
    virtual void BeganFrame(const mcs::TimestampUs &timestamp) = 0;
    virtual void FinishedFrame(const mcs::TimestampUs &timestamp) = 0;
    virtual void ReceivedInputBuffer(const mcs::TimestampUs &timestamp) = 0;
};

} // namespace video
} // namespace mcs

#endif
