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

#ifndef W11TNG_PROCESS_EXECUTOR_H_
#define W11TNG_PROCESS_EXECUTOR_H_

#include <memory>
#include <string>
#include <vector>

#include <ac/glib_wrapper.h>

#include <ac/non_copyable.h>

namespace w11tng {
class ProcessExecutor : public std::enable_shared_from_this<ProcessExecutor> {
public:
    typedef std::shared_ptr<ProcessExecutor> Ptr;

    class Delegate : public ac::NonCopyable {
    public:
        virtual void OnProcessTerminated() = 0;
    };

    static Ptr Create(const std::string &path, const std::vector<std::string> &argv, const std::weak_ptr<Delegate> &delegate);

    ~ProcessExecutor();

    bool Running() const { return pid_ > 0; }

private:
    ProcessExecutor(const std::weak_ptr<Delegate> &delegate);
    Ptr FinalizeConstruction(const std::string &path, const std::vector<std::string> &argv);

private:
    std::weak_ptr<Delegate> delegate_;
    GPid pid_;
    guint process_watch_;
};
} // namespace w11tng

#endif
