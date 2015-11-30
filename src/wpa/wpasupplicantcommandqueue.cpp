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

#include "wpasupplicantcommandqueue.h"

WpaSupplicantCommandQueue::WpaSupplicantCommandQueue(Delegate *delegate) :
    delegate_(delegate),
    current_(nullptr) {
}

void WpaSupplicantCommandQueue::EnqueueCommand(const WpaSupplicantMessage &message, WpaSupplicantCommand::ResponseCallback callback) {
    queue_.push(new WpaSupplicantCommand(message, callback));
    RestartQueue();
}

void WpaSupplicantCommandQueue::HandleMessage(WpaSupplicantMessage message) {
    if (message.Type() == kInvalid) {
        g_warning("Got invalid messsage");
        return;
    }

    if (message.Type() == kEvent) {
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

gboolean WpaSupplicantCommandQueue::OnRestartQueue(gpointer user_data) {
    auto inst = static_cast<WpaSupplicantCommandQueue*>(user_data);
    inst->CheckRestartingQueue();
}

void WpaSupplicantCommandQueue::RestartQueue() {
    g_idle_add(&WpaSupplicantCommandQueue::OnRestartQueue, this);
}

void WpaSupplicantCommandQueue::CheckRestartingQueue() {
    if (current_ != nullptr || queue_.size() == 0)
        return;

    WriteNextCommand();
    RestartQueue();
}

void WpaSupplicantCommandQueue::WriteNextCommand() {
    current_ = queue_.front();
    queue_.pop();

    if (!delegate_)
        return;

    current_->message.Seal();

    delegate_->OnWriteMessage(current_->message);

    if (!current_->callback) {
        delete current_;
        current_ = nullptr;
    }
}
