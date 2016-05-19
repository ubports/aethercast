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

#include "ac/common/threadedexecutorfactory.h"
#include "ac/common/threadedexecutor.h"

namespace ac {
namespace common {

Executor::Ptr ThreadedExecutorFactory::Create(const Executable::Ptr &executable) {
    return std::make_shared<ThreadedExecutor>(executable);
}

} // namespace common
} // namespace ac
