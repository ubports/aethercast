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

#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "logging.h"

namespace mcs {

static LogLevel current_level = kInfo;

void InitLogging(LogLevel level) {
    current_level = level;
}

void Log(LogLevel level, const char *format, ...) {
    va_list ap;
    if (level > current_level)
        return;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    printf("\n");
}

} // namespace mcs
