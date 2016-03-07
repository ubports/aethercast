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

#ifndef MCS_REPORT_NULL_SENDERREPORT_H_
#define MCS_REPORT_NULL_SENDERREPORT_H_

#include <memory>

#include "mcs/non_copyable.h"
#include "mcs/utils.h"

#include "mcs/video/senderreport.h"

namespace mcs {
namespace report {
namespace null {

class SenderReport : public video::SenderReport {
public:
    void SentPacket(const mcs::TimestampUs timestamp, size_t size);
};

} // namespace null
} // namespace report
} // namespace mcs

#endif
