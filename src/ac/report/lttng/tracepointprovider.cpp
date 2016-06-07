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

#include <dlfcn.h>

#include <boost/throw_exception.hpp>

#include "ac/report/lttng/tracepointprovider.h"

namespace
{
static const std::string kTracepointLibInstallPath{AETHERCAST_TRACEPOINT_LIB_INSTALL_PATH};
static const std::string kAethercastTpProviderLibName{"libaethercast-lttng.so"};

void* OpenTracepointLib(std::string const& lib_name) {
    auto lib = dlopen(lib_name.c_str(), RTLD_NOW);

    if (!lib) {
        std::string path{kTracepointLibInstallPath + "/" + lib_name};
        lib = dlopen(path.c_str(), RTLD_NOW);
    }

    if (!lib) {
        std::string msg{"Failed to load tracepoint provider: "};
        msg += dlerror();
        BOOST_THROW_EXCEPTION(std::runtime_error(msg));
    }

    return lib;
}
}

namespace ac {
namespace report {
namespace lttng {

TracepointProvider::TracepointProvider() :
    lib_(OpenTracepointLib(kAethercastTpProviderLibName)) {
}

TracepointProvider::~TracepointProvider() {
    dlclose(lib_);
}

} // namespace lttng
} // namespace report
} // namespace ac
