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

#ifndef UTILS_H_
#define UTILS_H_

#include <boost/format.hpp>

#include <string>
#include <vector>

namespace mcs {
struct Utils
{
    // Merely used as a namespace.
    Utils() = delete;

    // StringStartsWith returns true iff text[0:prefix.size()-1] == prefix.
    static bool StringStartsWith(const std::string &text, const std::string &prefix);
    // ParseHex parses an integer with base 16 from str.
    static int ParseHex(const std::string &str);
    // StringSplit splits text up for the given separator sep and returns a vector of all tokens.
    static std::vector<std::string> StringSplit(const std::string &text, char sep);
    // Sprintf - much like what you would expect :)
    template<typename... Types>
    static std::string Sprintf(const std::string& fmt_str, Types&&... args);
    // GetEnv - returns a variable value from the environment
    static std::string GetEnvValue(const std::string &name);
};

namespace impl {
// Base case, just return the passed in boost::format instance.
inline boost::format& Sprintf(boost::format& f)
{
    return f;
}
// Sprintf recursively walks the parameter pack at compile time.
template <typename Head, typename... Tail>
inline boost::format& Sprintf(boost::format& f, Head const& head, Tail&&... tail) {
    return Sprintf(f % head, std::forward<Tail>(tail)...);
}
} // namespace impl
} // namespace mcs

template <typename... Types>
inline std::string mcs::Utils::Sprintf(const std::string& format, Types&&... args) {
    boost::format f(format);
    return impl::Sprintf(f, std::forward<Types>(args)...).str();
}

#endif
