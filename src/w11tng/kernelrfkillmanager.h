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

#ifndef W11TNG_KERNEL_RFKILL_MANAGER_H_
#define W11TNG_KERNEL_RFKILL_MANAGER_H_

#include "w11tng/rfkillmanager.h"

#include <map>

namespace w11tng {
template<typename T>
struct GIOChannelDeleter {
    void operator()(T *object) const {
        if (object)
            g_io_channel_unref(object);
    }
};

class KernelRfkillManager : public RfkillManager,
                            public std::enable_shared_from_this<KernelRfkillManager> {
public:
    static Ptr Create();

    ~KernelRfkillManager();

    bool IsBlocked(const Type &type) const override;

private:
    KernelRfkillManager();

    Ptr FinalizeConstruction();
    bool ProcessRfkillEvents();

    static gboolean OnRfkillEvent(GIOChannel *channel, GIOCondition cond, gpointer data);

    std::unique_ptr<GIOChannel, GIOChannelDeleter<GIOChannel>> channel_;
    guint watch_;
    std::map<Type,bool> block_status_;
};
} // namespace w11tng

#endif
