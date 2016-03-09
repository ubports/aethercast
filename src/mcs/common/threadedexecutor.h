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

#ifndef MCS_COMMON_THREADEDEXECUTOR_H_
#define MCS_COMMON_THREADEDEXECUTOR_H_

#include <atomic>
#include <memory>
#include <thread>

#include "mcs/common/executor.h"
#include "mcs/common/executable.h"

namespace mcs {
namespace common {

class ThreadedExecutor : public Executor {
public:
    static Executor::Ptr Create(const Executable::Ptr &executable,
                                const std::string &name = "");

    ~ThreadedExecutor();

    bool Start() override;
    bool Stop() override;

    bool Running() const override;

private:
    ThreadedExecutor(const Executable::Ptr &executable, const std::string &name);

    void ThreadWorker();

private:
    Executable::Ptr executable_;
    std::string name_;
    std::atomic<bool> running_;
    std::thread thread_;
};

} // namespace common
} // namespace mcs

#endif
