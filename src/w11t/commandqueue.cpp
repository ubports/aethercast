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

#include <glib.h>

#include "commandqueue.h"

namespace w11t {
CommandQueue::CommandQueue(Delegate *delegate) :
    delegate_(delegate),
    current_(nullptr),
    idle_source_(0) {
}

CommandQueue::~CommandQueue() {
    if (idle_source_ > 0)
        g_source_remove(idle_source_);
}

void CommandQueue::EnqueueCommand(const Message &message, Command::ResponseCallback callback) {
    queue_.push(new Command(message, callback));
    RestartQueue();
}

void CommandQueue::HandleMessage(Message message) {
    if (message.ItsType() == Message::Type::kInvalid)
        return;

    if (message.ItsType() == Message::Type::kEvent) {
        if (delegate_)
            delegate_->OnUnsolicitedResponse(message);
        return;
    }

    if (!current_)
        return;

    if (current_->callback)
        current_->callback(message);

    delete current_;
    // This will unblock the queue and will allow us to send the next command
    current_ = nullptr;

    RestartQueue();
}

gboolean CommandQueue::OnRestartQueue(gpointer user_data) {
    auto inst = static_cast<CommandQueue*>(user_data);
    inst->idle_source_ = 0;
    inst->CheckRestartingQueue();
    return FALSE;
}

void CommandQueue::RestartQueue() {
    if (idle_source_ > 0)
        return;

    idle_source_ = g_idle_add(&CommandQueue::OnRestartQueue, this);
}

void CommandQueue::CheckRestartingQueue() {
    if (current_ != nullptr || queue_.size() == 0)
        return;

    WriteNextCommand();
    RestartQueue();
}

void CommandQueue::WriteNextCommand() {
    current_ = queue_.front();
    queue_.pop();

    if (!delegate_)
        return;

    if (!current_)
        return;

    current_->message.Seal();

    delegate_->OnWriteMessage(current_->message);

    if (!current_->callback) {
        delete current_;
        current_ = nullptr;
    }
}
}
