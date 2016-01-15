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

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <memory>
#include <fstream>

#include <cstring>
#include <cstdarg>

#include "utils.h"

namespace mcs {
bool Utils::StringStartsWith(const std::string &text, const std::string &prefix) {
    return text.compare(0, prefix.size(), prefix) == 0;
}

int Utils::ParseHex(const std::string &str) {
    // No need to manually skip hex indicating prefix, see:
    //   http://en.cppreference.com/w/cpp/string/basic_string/stol
    return std::stoi(str, nullptr, 16);
}

std::vector<std::string> Utils::StringSplit(const std::string &text, char sep) {
  std::vector<std::string> tokens;
  return boost::algorithm::split(tokens, text, boost::is_from_range(sep, sep), boost::algorithm::token_compress_on);
}

std::string Utils::GetEnvValue(const std::string &name) {
    char *value = getenv(name.c_str());
    if (!value)
        return std::string("");
    return std::string(value);
}

bool Utils::CreateFile(const std::string &file_path) {
    boost::filesystem::path p(file_path);
    if (boost::filesystem::exists(p))
        return false;

    std::ofstream of;
    of.open(file_path, std::ofstream::out);
    of << "";
    of.close();
    return true;
}
} // namespace mcs
