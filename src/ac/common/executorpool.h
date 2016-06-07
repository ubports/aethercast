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

#ifndef AC_COMMON_EXECUTORPOOL_H_
#define AC_COMMON_EXECUTORPOOL_H_

#include <cstddef>

#include <vector>

#include "ac/non_copyable.h"

#include "ac/common/executor.h"
#include "ac/common/executorfactory.h"
#include "ac/common/executable.h"

namespace ac {
namespace common {


class ExecutorPool : public ac::NonCopyable {
public:
    ExecutorPool(const ExecutorFactory::Ptr &factory, const size_t &size);
    ~ExecutorPool();

    bool Add(const Executable::Ptr &executable);

    bool Start();
    bool Stop();

    bool Running() const;

private:
    struct Item {
        Executable::Ptr executable;
        Executor::Ptr executor;
    };

    std::uint32_t size_;
    bool running_;
    ExecutorFactory::Ptr factory_;
    std::vector<Item> items_;
};

} // namespace common
} // namespace ac

#endif
