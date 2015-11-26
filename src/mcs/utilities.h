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

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <string>
#include <vector>

namespace utilities {

bool StringStartsWith(const std::string &text, const std::string &prefix);
int ParseHex(const std::string &str);
std::vector<std::string> StringSplit(const std::string &text, char sep);
std::string StringFormat(const std::string fmt_str, ...);

} // namespace utilities

#endif
