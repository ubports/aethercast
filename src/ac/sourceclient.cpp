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

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <vector>

#include "ac/keep_alive.h"
#include "ac/logger.h"
#include "ac/sourceclient.h"
#include "ac/mediamanagerfactory.h"
#include "ac/networkutils.h"
#include "ac/utils.h"
#include "ac/logger.h"

#include "ac/mir/sourcemediamanager.h"

#include "ac/network/udpstream.h"

namespace {
static int send_cseq = 0;
}

namespace ac {
std::shared_ptr<SourceClient> SourceClient::Create(ScopedGObject<GSocket>&& socket, const ac::IpV4Address &local_address) {
    std::shared_ptr<SourceClient> sp{new SourceClient{std::move(socket), local_address}};
    return sp->FinalizeConstruction();
}

SourceClient::SourceClient(ScopedGObject<GSocket>&& socket, const ac::IpV4Address &local_address) :
    socket_(std::move(socket)),
    socket_source_(0),
    local_address_(local_address) {
}

SourceClient::~SourceClient() {
    if (socket_source_ > 0)
        g_source_remove(socket_source_);

    ReleaseTimers();
}

void SourceClient::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

void SourceClient::ResetDelegate() {
    delegate_.reset();
}

void SourceClient::DumpRtsp(const std::string &prefix, const std::string &data) {
    static bool enabled = getenv("AETHERCAST_RTSP_DEBUG") != nullptr;

    if (!enabled)
        return;

    auto lines = Utils::StringSplit(data, '\n');
    for (auto current : lines)
        AC_WARNING("RTSP: %s: %s", prefix.c_str(), current.c_str());
}

void SourceClient::SendRTSPData(const std::string &data) {
    DumpRtsp("OUT", data);
    GError *error = nullptr;
    auto bytes_written = g_socket_send(socket_.get(), data.c_str(), data.length(), nullptr, &error);
    if (bytes_written < 0) {
        AC_WARNING("Failed to write data to RTSP client: %s", error->message);
        g_error_free(error);
        return;
    }
}

std::string SourceClient::GetLocalIPAddress() const {
    return local_address_.to_string();
}

int SourceClient::GetNextCSeq(int *initial_peer_cseq) const {
    ++send_cseq;
    if (initial_peer_cseq && send_cseq == *initial_peer_cseq)
        send_cseq *= 2;
    return send_cseq;
}

class TimerCallbackData {
public:
    TimerCallbackData(SourceClient *delegate) :
        delegate_(delegate),
        id_(0) {
    }

    guint id_;
    SourceClient *delegate_;
};

uint SourceClient::CreateTimer(int seconds) {
    auto data = new TimerCallbackData(this);
    auto id = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT, seconds,
                                         &SourceClient::OnTimeout, data,
                                         &SourceClient::OnTimeoutRemove);
    if (id > 0) {
        data->id_ = id;
        timers_.push_back(id);
    } else {
        delete data;
    }

    return id;
}

void SourceClient::ReleaseTimer(uint timer_id) {
    if (timer_id <= 0)
        return;

    auto it = std::find(timers_.begin(), timers_.end(), timer_id);
    if (it != timers_.end())
        g_source_remove(*it);
}

void SourceClient::ReleaseTimers() {
    for (auto timer : timers_)
        g_source_remove(timer);

    timers_.clear();
}

gboolean SourceClient::OnTimeout(gpointer user_data) {
    auto data = static_cast<TimerCallbackData*>(user_data);
    data->delegate_->source_->OnTimerEvent(data->id_);
    return FALSE;
}

void SourceClient::OnTimeoutRemove(gpointer user_data) {
    auto data = static_cast<TimerCallbackData*>(user_data);
    auto& timers = data->delegate_->timers_;
    auto it = std::find(timers.begin(), timers.end(), data->id_);
    if (it != timers.end())
        timers.erase(it);
    delete data;
}

gboolean SourceClient::OnIncomingData(GSocket *socket, GIOCondition  cond, gpointer user_data) {
    auto inst = static_cast<WeakKeepAlive<SourceClient>*>(user_data)->GetInstance().lock();

    if (cond == G_IO_ERR || cond == G_IO_HUP) {
        if (auto sp = inst->delegate_.lock())
            sp->OnConnectionClosed();
        return TRUE;
    }

    int fd = g_socket_get_fd(inst->socket_.get());
    while (g_socket_get_available_bytes(inst->socket_.get()) > 0) {
        gchar buf[1024] = { };
        gssize nbytes = g_socket_receive(inst->socket_.get(), buf, 1024, NULL, NULL);
        if (nbytes <= 0)
            break;

        std::string data(buf);
        inst->DumpRtsp("IN", data);

        inst->source_->RTSPDataReceived(data);
    }

    return TRUE;
}

std::shared_ptr<SourceClient> SourceClient::FinalizeConstruction() {
    auto sp = shared_from_this();

    GError *error = nullptr;
    auto address = g_socket_get_remote_address(socket_.get(), &error);
    if (error) {
        AC_WARNING("Failed to receive address from socket: %s", error->message);
        g_error_free(error);
        return sp;
    }

    auto inet_address = g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(address));
    if (!inet_address) {
        AC_WARNING("Failed to determine client address");
        return sp;
    }

    std::string peer_address = g_inet_address_to_string(G_INET_ADDRESS(inet_address));

    auto source = g_socket_create_source(socket_.get(), static_cast<GIOCondition>((G_IO_IN | G_IO_HUP | G_IO_ERR)), nullptr);
    if (!source) {
        AC_WARNING("Failed to setup event listener for source client");
        return sp;
    }

    g_source_set_callback(source, (GSourceFunc) &SourceClient::OnIncomingData,
                          new WeakKeepAlive<SourceClient>{sp},
                          [](gpointer data) { delete static_cast<WeakKeepAlive<SourceClient>*>(data); });
    socket_source_ = g_source_attach(source, nullptr);
    if (socket_source_ == 0) {
        AC_WARNING("Failed to attach source to mainloop");
        g_source_unref(source);
        return sp;
    }

    auto udp_stream = std::make_shared<ac::network::UdpStream>();

    media_manager_ = MediaManagerFactory::CreateSource(peer_address, udp_stream);
    media_manager_->SetDelegate(shared_from_this());
    source_.reset(wds::Source::Create(this, media_manager_.get(), this));

    source_->Start();
    return sp;
}

void SourceClient::NotifyConnectionClosed() {
    if (auto sp = delegate_.lock())
        sp->OnConnectionClosed();
}

void SourceClient::OnSourceNetworkError() {
    NotifyConnectionClosed();
}

void SourceClient::ErrorOccurred(wds::ErrorType error) {
    if (error != wds::ErrorType::TimeoutError)
        return;

    AC_ERROR("Detected RTSP timeout; disconnecting ..");

    NotifyConnectionClosed();
}

void SourceClient::SessionCompleted() {
    AC_DEBUG("");
}

} // namespace ac
