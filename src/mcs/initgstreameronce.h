/*
 * Copyright (C) 2015 Canonical, Ltd.
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

#include <gst/gst.h>

#include "logger.h"

namespace mcs {
// GstDebugLevelToSeverity maps a GstDebugLevel to a Logger::Severity;
Logger::Severity GstDebugLevelToSeverity(GstDebugLevel);

// InitGstreamerOnceOrThrow initializes GStreamer, redirecting
// its debug output to the mcs::Logger facilities.
void InitGstreamerOnceOrThrow();
}
