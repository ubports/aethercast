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

#include <boost/concept_check.hpp>

#include "ac/logger.h"

#include "ac/report/null/senderreport.h"

namespace ac {
namespace report {
namespace null {

void SenderReport::SentPacket(const TimestampUs &timestamp, const size_t &size) {
    boost::ignore_unused_variable_warning(timestamp);
    boost::ignore_unused_variable_warning(size);
}

} // namespace null
} // namespace report
} // namespace ac
