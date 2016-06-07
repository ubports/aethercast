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

#ifndef AC_COMMON_EXECUTOR_H_
#define AC_COMMON_EXECUTOR_H_

#include <memory>

#include "ac/non_copyable.h"

namespace ac {
namespace common {

class Executor : public ac::NonCopyable {
public:
    typedef std::shared_ptr<Executor> Ptr;

    virtual bool Start() = 0;
    virtual bool Stop() = 0;
    virtual bool Running() const = 0;

protected:
    Executor() = default;
};

} // namespace common
} // namespace ac

#endif
