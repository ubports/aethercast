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

#include <wds/peer.h>
#include <wds/source.h>
#include <wds/media_manager.h>

#include "ac/glib_wrapper.h"
#include "ac/ip_v4_address.h"
#include "ac/non_copyable.h"
#include "ac/scoped_gobject.h"
#include "ac/basesourcemediamanager.h"

namespace ac {
class TimerCallbackData;

class SourceClient : public std::enable_shared_from_this<SourceClient>,
                     public wds::Peer::Delegate,
                     public wds::Peer::Observer,
                     public BaseSourceMediaManager::Delegate {
public:
    class Delegate : private ac::NonCopyable {
    public:
        virtual void OnConnectionClosed() = 0;
    };

    static std::shared_ptr<SourceClient> Create(ScopedGObject<GSocket>&& socket, const ac::IpV4Address &local_address);

    ~SourceClient();

    void SetDelegate(const std::weak_ptr<Delegate>& delegate);
    void ResetDelegate();

    void OnSourceNetworkError();

public:
    void SendRTSPData(const std::string &data) override;
    std::string GetLocalIPAddress() const override;
    uint CreateTimer(int seconds) override;
    void ReleaseTimer(uint timerId) override;
    int GetNextCSeq(int* initial_peer_cseq = nullptr) const override;

public:
    void ErrorOccurred(wds::ErrorType error) override;
    void SessionCompleted() override;

public:
    static gboolean OnTimeout(gpointer user_data);
    static void OnTimeoutRemove(gpointer user_data);
    static gboolean OnIncomingData(GSocket *socket, GIOCondition condition,
                                     gpointer user_data);

private:
    SourceClient(ScopedGObject<GSocket>&& socket, const ac::IpV4Address &local_address);
    std::shared_ptr<SourceClient> FinalizeConstruction();

    void DumpRtsp(const std::string &prefix, const std::string &data);
    void ReleaseTimers();
    void NotifyConnectionClosed();

private:
    std::weak_ptr<Delegate> delegate_;
    ScopedGObject<GSocket> socket_;
    guint socket_source_;
    ac::IpV4Address local_address_;
    std::vector<guint> timers_;
    std::unique_ptr<wds::Source> source_;
    std::shared_ptr<BaseSourceMediaManager> media_manager_;
    guint watch_;

    friend class TimerCallbackData;
};
} // namespace ac
#endif
