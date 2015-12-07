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

#ifndef LOGGING_H_
#define LOGGING_H_

namespace mcs {

enum LogLevel {
    kError = 0,
    kWarning,
    kInfo,
    kDebug
};

void InitLogging(LogLevel level);
void Log(LogLevel level, const char *format, ...);

#define Debug(format, args...) \
    Log(mcs::kDebug, "%s:%s() " format, __FILE__, __FUNCTION__, ## args)

#define Info(format, args...) \
    Log(mcs::kInfo, "%s:%s() " format, __FILE__, __FUNCTION__, ## args)

#define Warning(format, args...) \
    Log(mcs::kWarning, "%s:%s() " format, __FILE__, __FUNCTION__, ## args)

#define Error(format, args...) \
    Log(mcs::kError, "%s:%s() " format, __FILE__, __FUNCTION__, ## args)

} // namespace mcs

#endif
