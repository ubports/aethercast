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

#include "keep_alive.h"
#include "miracastsourcemanager.h"
#include "miracastsourceclient.h"
#include "logging.h"

namespace mcs {
std::shared_ptr<MiracastSourceManager> MiracastSourceManager::create() {
    return std::shared_ptr<MiracastSourceManager>{new MiracastSourceManager{}};
}

MiracastSourceManager::MiracastSourceManager() :
    active_sink_(nullptr),
    socket_(nullptr),
    socket_source_(0) {
}

MiracastSourceManager::~MiracastSourceManager() {
    Release();
}

void MiracastSourceManager::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

void MiracastSourceManager::ResetDelegate() {
    delegate_.reset();
}

bool MiracastSourceManager::Setup(const IpV4Address &address, unsigned short port) {
    GError *error;

    if (socket_)
        return false;

    ScopedGObject<GSocket> socket{g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, &error)};

    if (!socket) {
        mcs::Error("Failed to setup socket for incoming source connections: %s", error->message);
        g_error_free(error);
        return false;
    }

    auto addr = g_inet_socket_address_new_from_string(address.to_string().c_str(), port);
    if (!g_socket_bind(socket.get(), addr, TRUE, &error)) {
        mcs::Error("Failed to setup socket for incoming source connections: %s", error->message);
        g_error_free(error);
        return false;
    }

    if (!g_socket_listen(socket.get(), &error)) {
        mcs::Error("Failed start listening for incoming connections: %s", error->message);
        g_error_free(error);
        return false;
    }

    auto source = g_socket_create_source(socket.get(), G_IO_IN, nullptr);
    if (!source) {
        mcs::Error("Failed to setup listener for incoming connections");
        return false;
    }

    g_source_set_callback(source, (GSourceFunc) &MiracastSourceManager::OnNewConnection,
                          new WeakKeepAlive<MiracastSourceManager>(shared_from_this()),
                          [](gpointer data) {
                              delete static_cast<WeakKeepAlive<MiracastSourceManager>*>(data);
                          });

    socket_source_ = g_source_attach(source, nullptr);
    if (socket_source_ == 0) {
        mcs::Error("Failed to attach source to mainloop");
        g_source_unref(source);
        return false;
    }

    g_source_unref(source);

    mcs::Debug("Successfully setup source on %s:%d and awaiting incoming connection requests",
                address.to_string().c_str(), port);

    socket_.swap(socket);

    return true;
}

void MiracastSourceManager::Release() {
    if (socket_source_ > 0) {
        g_source_remove(socket_source_);
        socket_source_ = 0;
    }

    active_sink_.reset();
}

gboolean MiracastSourceManager::OnNewConnection(GSocket *socket, GIOCondition  cond, gpointer user_data) {
    auto inst = static_cast<WeakKeepAlive<MiracastSourceManager>*>(user_data)->GetInstance().lock();

    // The callback context was deleted while the wait for connection was active.
    // Hardly anything we can do about it except for returning early.
    if (not inst)
        return FALSE;

    GError *error = nullptr;
    auto client_socket = g_socket_accept(inst->socket_.get(), NULL, &error);
    if (!client_socket) {
        mcs::Error("Failed to accept incoming connection: %s", error->message);
        g_error_free(error);
        g_object_unref(client_socket);
        return FALSE;
    }

    // If we add support for a coupled sink some day we can allow
    // more than one client to connect here.
    if (inst->active_sink_) {
        g_socket_close(client_socket, nullptr);
        g_object_unref(client_socket);
        return FALSE;
    }

    inst->active_sink_ = MiracastSourceClient::create(ScopedGObject<GSocket>{client_socket});
    inst->active_sink_->SetDelegate(inst->shared_from_this());

    return FALSE;
}

void MiracastSourceManager::OnConnectionClosed() {
    if (auto sp = delegate_.lock())
        sp->OnClientDisconnected();
}
} // namespace mcs
