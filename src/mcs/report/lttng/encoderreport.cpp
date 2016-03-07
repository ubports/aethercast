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

#include "mcs/report/lttng/encoderreport.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "mcs/report/lttng/encoderreport_tp.h"

namespace mcs {
namespace report {
namespace lttng {

void EncoderReport::Started() {
    mcs_tracepoint(aethercast_encoder, started, 0);
}

void EncoderReport::Stopped() {
    mcs_tracepoint(aethercast_encoder, stopped, 0);
}

void EncoderReport::BeganFrame(const mcs::TimestampUs timestamp) {
    mcs_tracepoint(aethercast_encoder, began_frame, timestamp);
}

void EncoderReport::FinishedFrame(const mcs::TimestampUs timestamp) {
    mcs_tracepoint(aethercast_encoder, finished_frame, timestamp);
}

void EncoderReport::ReceivedInputBuffer(const mcs::TimestampUs timestamp) {
    mcs_tracepoint(aethercast_encoder, received_input_buffer, timestamp);
}

} // namespace logging
} // namespace report
} // namespace mcs
