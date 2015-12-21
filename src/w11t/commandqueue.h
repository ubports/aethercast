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

#ifndef WPASUPPLICANTCOMMANDQUEUE_H_
#define WPASUPPLICANTCOMMANDQUEUE_H_

#include <boost/noncopyable.hpp>

#include <string>
#include <queue>

#include <mcs/non_copyable.h>

#include "command.h"

namespace w11t {
class CommandQueue {
public:
    class Delegate : private mcs::NonCopyable {
    public:
        virtual void OnUnsolicitedResponse(Message message) = 0;
        virtual void OnWriteMessage(Message message) = 0;

    protected:
        Delegate() = default;
    };

    CommandQueue(Delegate *delegate);
    ~CommandQueue();

    void EnqueueCommand(const Message &message, Command::ResponseCallback callback = nullptr);
    void HandleMessage(Message message);

private:
    void RestartQueue();
    void CheckRestartingQueue();
    void WriteNextCommand();

private:
    static gboolean OnRestartQueue(gpointer user_data);

private:
    Delegate *delegate_;
    Command *current_;
    std::queue<Command*> queue_;
    unsigned int idle_source_;
};
}
#endif
