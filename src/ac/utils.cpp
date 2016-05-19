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
#include <sstream>
#include <iostream>

#include <cstring>
#include <cstdarg>
#include <cstdio>

#include "utils.h"

namespace ac {
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

std::string Utils::GetEnvValue(const std::string &name, const std::string &default_value) {
    char *value = getenv(name.c_str());
    if (!value)
        return default_value;
    return std::string(value);
}

bool Utils::IsEnvSet(const std::string &name) {
    return getenv(name.c_str()) != nullptr;
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

bool Utils::RemoveFile(const std::string &file_path) {
    boost::filesystem::path p(file_path);
    if (!boost::filesystem::exists(p))
        return false;

    ::remove(file_path.c_str());
    return true;
}

uint64_t Utils::GetNowNs() {
   struct timespec ts;
   memset(&ts, 0, sizeof(ts));
   clock_gettime(CLOCK_MONOTONIC, &ts);
   return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

uint64_t Utils::GetNowUs() {
    return GetNowNs() / 1000;
}

std::string Utils::Hexdump(const uint8_t *data, uint32_t size) {
    unsigned char buff[17];
    const uint8_t *pc = data;
    std::stringstream buffer;

    if (size == 0) {
        buffer << "NULL" << std::endl;
        return buffer.str();
    }

    uint32_t i;
    for (i = 0; i < size; i++) {
        if ((i % 16) == 0) {
            if (i != 0)
                buffer << ac::Utils::Sprintf("  %s", buff) << std::endl;

            buffer << ac::Utils::Sprintf("%02x   ", i);
        }

        buffer << ac::Utils::Sprintf(" %02x", static_cast<int>(pc[i]));

        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0) {
        buffer << "   ";
        i++;
    }

    buffer << ac::Utils::Sprintf("  %s", buff) << std::endl;

    return buffer.str();
}

void Utils::SetThreadName(const std::string &name) {
    pthread_setname_np(pthread_self(), name.c_str());
}

} // namespace ac
