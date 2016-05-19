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

#ifndef W11TNG_RFKILL_MANAGER_H_
#define W11TNG_RFKILL_MANAGER_H_

#include <memory>
#include <map>

#include "ac/non_copyable.h"
#include "ac/glib_wrapper.h"

namespace w11tng {

template<typename T>
struct GIOChannelDeleter {
    void operator()(T *object) const {
        if (object)
            g_io_channel_unref(object);
    }
};

class RfkillManager : public std::enable_shared_from_this<RfkillManager> {
public:
    typedef std::shared_ptr<RfkillManager> Ptr;

    enum class Type {
        kAll = 0,
        kWLAN,
        kBluetooth,
        kUWB,
        kWWAN,
        kGPS,
        kFM,
    };

    class Delegate : public ac::NonCopyable {
    public:
        virtual void OnRfkillChanged(const Type &type) = 0;
    };

    static Ptr Create();

    ~RfkillManager();

    void SetDelegate(const std::weak_ptr<Delegate> &delegate);
    void ResetDelegate();

    bool IsBlocked(const Type &type);

private:
    RfkillManager();

    Ptr FinalizeConstruction();

    bool ProcessRfkillEvents();

    static gboolean OnRfkillEvent(GIOChannel *channel, GIOCondition cond, gpointer data);

private:
    std::weak_ptr<Delegate> delegate_;
    std::unique_ptr<GIOChannel, GIOChannelDeleter<GIOChannel>> channel_;
    guint watch_;
    std::map<Type,bool> block_status_;
};
} // namespace w11tng

#endif
