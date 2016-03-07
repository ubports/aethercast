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

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER aethercast_sender

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "mcs/report/lttng/senderreport_tp.h"

#if !defined(MCS_REPORT_LTTNG_SENDERREPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MCS_REPORT_LTTNG_SENDERREPORT_TP_H_

#include "mcs/report/lttng/utils.h"

MCS_LTTNG_VOID_TRACE_CLASS(TRACEPOINT_PROVIDER)

#define ENCODER_TRACE_POINT(name) MCS_LTTNG_VOID_TRACE_POINT(TRACEPOINT_PROVIDER, name)

TRACEPOINT_EVENT(
    TRACEPOINT_PROVIDER,
    sent_packet,
    TP_ARGS(int, timestamp, int, size),
    TP_FIELDS(
        ctf_integer(int, timestamp, timestamp)
        ctf_integer(int, size, size)
    )
)

#undef ENCODER_TRACE_POINT

#endif

#include <lttng/tracepoint-event.h>
