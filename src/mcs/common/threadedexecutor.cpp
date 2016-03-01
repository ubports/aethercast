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

#include "mcs/utils.h"
#include "mcs/logger.h"

#include "mcs/common/threadedexecutor.h"

namespace mcs {
namespace common {

Executor::Ptr ThreadedExecutor::Create(const Executable::Ptr &executable, const std::string &name) {
    return std::shared_ptr<Executor>(new ThreadedExecutor(executable, name));
}

ThreadedExecutor::ThreadedExecutor(const Executable::Ptr &executable, const std::string &name) :
    executable_(executable),
    name_(name),
    running_(false) {
}

ThreadedExecutor::~ThreadedExecutor() {
    Stop();
}

void ThreadedExecutor::ThreadWorker() {
    if (name_.length() > 0) {
        mcs::Utils::SetThreadName(name_);
        MCS_DEBUG("Started threaded executor %s", name_);
    }

    while (running_) {
        if (!executable_->Execute())
            break;
    }
}

bool ThreadedExecutor::Start() {
    if (running_)
        return false;

    running_ = true;
    thread_ = std::thread(&ThreadedExecutor::ThreadWorker, this);

    return true;
}

bool ThreadedExecutor::Stop() {
    if (!running_)
        return false;

    running_ = false;
    thread_.join();

    return true;
}

bool ThreadedExecutor::Running() const {
    return running_;
}

} // namespace common
} // namespace mcs
