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

#ifndef MCS_COMMON_EXECUTORPOOL_H_
#define MCS_COMMON_EXECUTORPOOL_H_

#include <cstddef>

#include <vector>

#include "mcs/non_copyable.h"

#include "mcs/common/executor.h"
#include "mcs/common/executorfactory.h"
#include "mcs/common/executable.h"

namespace mcs {
namespace common {


class ExecutorPool : public mcs::NonCopyable {
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
} // namespace mcs

#endif
