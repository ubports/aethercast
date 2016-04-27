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

#include <string.h>
#include <errno.h>

#include "mcs/logger.h"
#include "mcs/keep_alive.h"

#include "rfkillmanager.h"

namespace {
constexpr const char *kDevRfkillPath{"/dev/rfkill"};

struct RfkillEvent {
    uint32_t idx;
    uint8_t type;
    uint8_t op;
    uint8_t soft;
    uint8_t hard;
};

enum RfkillOp {
    kRfkillOpAdd = 0,
    kRfkillOpDel,
    kRfkillOpChange,
    kRfkillOpChangeAll,
};
}

namespace w11tng {

RfkillManager::Ptr RfkillManager::Create() {
    return std::shared_ptr<RfkillManager>(new RfkillManager)->FinalizeConstruction();
}

RfkillManager::RfkillManager() :
    watch_(0) {
}

RfkillManager::~RfkillManager() {
    if (watch_)
        g_source_remove(watch_);
}

RfkillManager::Ptr RfkillManager::FinalizeConstruction() {
    auto sp = shared_from_this();

    auto fd = ::open(kDevRfkillPath, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        MCS_ERROR("Failed to open rfkill device at %s: %s",
                  kDevRfkillPath, ::strerror(errno));
        return sp;
    }

    channel_.reset(g_io_channel_unix_new(fd));
    g_io_channel_set_close_on_unref(channel_.get(), TRUE);
    g_io_channel_set_encoding(channel_.get(), nullptr, nullptr);
    g_io_channel_set_buffered(channel_.get(), FALSE);

    int flags = g_io_channel_get_flags(channel_.get());
    flags |= G_IO_FLAG_NONBLOCK;
    g_io_channel_set_flags(channel_.get(), GIOFlags(flags), nullptr);

    while (ProcessRfkillEvents());

    watch_ = g_io_add_watch_full(channel_.get(), G_PRIORITY_DEFAULT,
                GIOCondition(G_IO_IN | G_IO_NVAL | G_IO_HUP | G_IO_ERR),
                OnRfkillEvent,
                new mcs::WeakKeepAlive<RfkillManager>(sp),
                [](gpointer user_data) { delete static_cast<mcs::WeakKeepAlive<RfkillManager>*>(user_data); });

    return sp;
}

void RfkillManager::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

void RfkillManager::ResetDelegate() {
    delegate_.reset();
}

bool RfkillManager::IsBlocked(const Type &type) {
    if (block_status_.find(type) == block_status_.end())
        return false;

    return block_status_[type];
}

bool RfkillManager::ProcessRfkillEvents() {
    uint8_t buf[32];
    RfkillEvent *event = reinterpret_cast<RfkillEvent*>(buf);

    ::memset(buf, 0, sizeof(buf));

    gsize len = 0;
    auto status = g_io_channel_read_chars(channel_.get(), reinterpret_cast<gchar*>(buf),
                                          sizeof(RfkillEvent),
                                          &len, nullptr);

    if (status != G_IO_STATUS_NORMAL)
        return false;

    if (len != sizeof(RfkillEvent))
        return false;

    auto type = static_cast<Type>(event->type);

    switch (event->op) {
    case RfkillOp::kRfkillOpAdd:
    case RfkillOp::kRfkillOpChange:

        block_status_[type] = (event->soft || event->hard);

        MCS_DEBUG("rfkill type %d is now %s",
                  static_cast<int>(event->type),
                  block_status_[type] ? "blocked" : "not blocked");

        if (auto sp = delegate_.lock())
            sp->OnRfkillChanged(type);
        break;
    default:
        break;
    }

    return true;
}

gboolean RfkillManager::OnRfkillEvent(GIOChannel *channel, GIOCondition cond, gpointer data) {
    auto thiz = static_cast<mcs::WeakKeepAlive<RfkillManager>*>(data)->GetInstance().lock();
    if (!thiz)
        return FALSE;

    if (cond & (G_IO_NVAL | G_IO_HUP | G_IO_ERR))
        return FALSE;

    return static_cast<gboolean>(thiz->ProcessRfkillEvents());
}

} // namespace w11tng
