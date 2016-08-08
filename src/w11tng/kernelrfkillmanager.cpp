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

#include "w11tng/kernelrfkillmanager.h"

#include "ac/logger.h"
#include "ac/keep_alive.h"

#include <string.h>
#include <errno.h>

namespace {
constexpr const char *kDevRfkillPath{"/dev/rfkill"};

enum class RfkillOp : std::uint8_t {
    kRfkillOpAdd = 0,
    kRfkillOpDel = 1,
    kRfkillOpChange = 2,
    kRfkillOpChangeAll = 3,
};

struct RfkillEvent {
    uint32_t idx;
    uint8_t type;
    RfkillOp op;
    uint8_t soft;
    uint8_t hard;
};
}

namespace w11tng {
KernelRfkillManager::Ptr KernelRfkillManager::Create() {
    return std::shared_ptr<KernelRfkillManager>(new KernelRfkillManager)->FinalizeConstruction();
}

KernelRfkillManager::KernelRfkillManager() :
    watch_(0) {
}

KernelRfkillManager::~KernelRfkillManager() {
    if (watch_)
        g_source_remove(watch_);
}

KernelRfkillManager::Ptr KernelRfkillManager::FinalizeConstruction() {
    auto sp = shared_from_this();

    auto fd = ::open(kDevRfkillPath, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        AC_ERROR("Failed to open rfkill device at %s: %s",
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

    // We're reading the current state of all rfkill items to make sure
    // have everything in sync
    while (ProcessRfkillEvents());

    watch_ = g_io_add_watch_full(channel_.get(), G_PRIORITY_DEFAULT,
                GIOCondition(G_IO_IN | G_IO_NVAL | G_IO_HUP | G_IO_ERR),
                OnRfkillEvent,
                new ac::WeakKeepAlive<KernelRfkillManager>(sp),
                [](gpointer user_data) { delete static_cast<ac::WeakKeepAlive<KernelRfkillManager>*>(user_data); });

    return sp;
}

bool KernelRfkillManager::ProcessRfkillEvents() {
    RfkillEvent event;
    ::memset(&event, 0, sizeof(RfkillEvent));

    gsize len = 0;
    auto status = g_io_channel_read_chars(channel_.get(), reinterpret_cast<gchar*>(&event),
                                          sizeof(RfkillEvent),
                                          &len, nullptr);

    if (status != G_IO_STATUS_NORMAL)
        return false;

    if (len != sizeof(RfkillEvent))
        return false;

    auto type = static_cast<Type>(event.type);

    switch (event.op) {
    case RfkillOp::kRfkillOpAdd:
    case RfkillOp::kRfkillOpChange:

        block_status_[type] = (event.soft || event.hard);

        AC_DEBUG("rfkill type %d is now %s",
                  static_cast<int>(event.type),
                  block_status_[type] ? "blocked" : "not blocked");

        if (auto sp = delegate_.lock())
            sp->OnRfkillChanged(type);
        break;
    default:
        break;
    }

    return true;
}

gboolean KernelRfkillManager::OnRfkillEvent(GIOChannel *channel, GIOCondition cond, gpointer data) {
    auto thiz = static_cast<ac::WeakKeepAlive<KernelRfkillManager>*>(data)->GetInstance().lock();
    if (!thiz)
        return FALSE;

    if (cond & (G_IO_NVAL | G_IO_HUP | G_IO_ERR))
        return FALSE;

    return static_cast<gboolean>(thiz->ProcessRfkillEvents());
}

bool KernelRfkillManager::IsBlocked(const Type &type) const {
    auto it = block_status_.find(type);
    return it == block_status_.end() ? false : it->second;
}
} // namespace w11tng
