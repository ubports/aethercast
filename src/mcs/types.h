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

#ifndef TYPES_H_
#define TYPES_H_

#include <string>

namespace mcs {

enum class Error {
    kNone,
    kFailed,
    kAlready,
    kParamInvalid,
    kInvalidState,
    kNoDeviceConnected,
    kUnknown
};

static std::string ErrorToString(const Error &error) {
    switch (error) {
    case Error::kNone:
        return "None";
    case Error::kFailed:
        return "Operation failed";
    case Error::kAlready:
        return "Operation already in progress";
    case Error::kParamInvalid:
        return "Invalid parameters";
    case Error::kInvalidState:
        return "Invalid state";
    case Error::kNoDeviceConnected:
        return "No device connected";
    case Error::kUnknown:
    default:
        break;
    }
    return "Unknown error occured";
}

typedef std::function<void(const Error &error)> ResultCallback;

} // namespace mcs

#endif
