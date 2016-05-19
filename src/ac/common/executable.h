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

#ifndef AC_COMMON_EXECUTABLE_H_
#define AC_COMMON_EXECUTABLE_H_

#include <memory>
#include <string>

#include "ac/non_copyable.h"

namespace ac {
namespace common {

class Executable : public ac::NonCopyable {
public:
    typedef std::shared_ptr<Executable> Ptr;

    virtual bool Start() = 0;
    virtual bool Stop() = 0;

    // Run one iteration and then return. Returning true means executor
    // should continue to execute otherwise false means it should stop.
    virtual bool Execute() = 0;

    virtual std::string Name() const = 0;

protected:
    Executable() = default;
};

} // namespace common
} // namespace ac

#endif
