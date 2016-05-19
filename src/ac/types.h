/*
 * Copyright (C) 2015-2016 Canonical, Ltd.
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

#ifndef TYPES_H_
#define TYPES_H_

#include <string>
#include <functional>

namespace ac {

enum class Error {
    kNone,
    kFailed,
    kAlready,
    kParamInvalid,
    kInvalidState,
    kNotConnected,
    kNotReady,
    kUnknown
};

typedef std::function<void(const Error &error)> ResultCallback;

std::string ErrorToString(const Error &error);

} // namespace ac

#endif
