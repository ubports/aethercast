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

#ifndef MIRACASTSOURCECLIENT_H_
#define MIRACASTSOURCECLIENT_H_

#include <boost/noncopyable.hpp>

#include <string>
#include <memory>
#include <map>

#include <glib.h>
#include <gio/gio.h>

#include <wds/peer.h>
#include <wds/source.h>
#include <wds/media_manager.h>

#include "scoped_gobject.h"

namespace mcs {
class TimerCallbackData;

class MiracastSourceClient : public wds::Peer::Delegate {
public:
    class Delegate : private boost::noncopyable {
    public:
        virtual void OnConnectionClosed() = 0;
    };

    MiracastSourceClient(Delegate *delegate, ScopedGObject<GSocket>&& socket);
    ~MiracastSourceClient();

public:
    void SendRTSPData(const std::string &data) override;
    std::string GetLocalIPAddress() const override;
    uint CreateTimer(int seconds) override;
    void ReleaseTimer(uint timerId) override;

public:
    static gboolean OnTimeout(gpointer user_data);
    static void OnTimeoutRemove(gpointer user_data);
    static gboolean OnIncomingData(GSocket *socket, GIOCondition condition,
                                     gpointer user_data);

private:
    void DumpRtsp(const std::string &prefix, const std::string &data);

private:
    Delegate *delegate_;
    ScopedGObject<GSocket> socket_;
    guint socket_source_;
    std::string local_address_;
    std::vector<guint> timers_;
    std::unique_ptr<wds::Source> source_;
    std::unique_ptr<wds::SourceMediaManager> media_manager_;
    guint watch_;

    friend class TimerCallbackData;
};
} // namespace mcs
#endif
