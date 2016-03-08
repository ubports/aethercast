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

#include "mcs/logger.h"

#include "mcs/report/logging/senderreport.h"

namespace mcs {
namespace report {
namespace logging {

void SenderReport::SentPacket(const TimestampUs &timestamp, const size_t &size) {
    MCS_TRACE("timestamp %lld size %d", timestamp, size);
}

} // namespace logging
} // namespace report
} // namespace mcs
