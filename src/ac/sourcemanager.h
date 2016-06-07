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

#ifndef MIRACASTSOURCE_H_
#define MIRACASTSOURCE_H_

#include <memory>

#include <boost/noncopyable.hpp>

#include "ac/glib_wrapper.h"
#include "ac/sourceclient.h"
#include "ac/scoped_gobject.h"
#include "ac/ip_v4_address.h"

namespace ac {
class SourceManager : public std::enable_shared_from_this<SourceManager>,
                      public SourceClient::Delegate {
public:
    class Delegate : private ac::NonCopyable {
    public:
        virtual void OnClientDisconnected() = 0;

    protected:
        Delegate() = default;
    };

    static std::shared_ptr<SourceManager> Create(const ac::IpV4Address &address, unsigned short port);

    ~SourceManager();

    void SetDelegate(const std::weak_ptr<Delegate> &delegate);
    void ResetDelegate();

public:
    void OnConnectionClosed();

private:
    static gboolean OnNewConnection(GSocket *socket, GIOCondition  cond, gpointer user_data);

    SourceManager();

    bool Setup(const ac::IpV4Address &address, unsigned short port);

private:
    std::weak_ptr<Delegate> delegate_;
    ScopedGObject<GSocket> socket_;
    guint socket_source_;
    std::shared_ptr<SourceClient> active_sink_;
    ac::IpV4Address local_address_;
};
} // namespace ac
#endif
