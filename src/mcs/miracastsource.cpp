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

#include "miracastsource.h"
#include "miracastsourceclient.h"

namespace mcs {
MiracastSource::MiracastSource(Delegate *delegate) :
    delegate_(delegate),
    active_sink_(nullptr),
    socket_(nullptr),
    socket_source_(0) {
}

MiracastSource::~MiracastSource() {
    Release();
}

bool MiracastSource::Setup(const std::string &address, unsigned short port) {
    GError *error;

    if (socket_)
        return false;

    ScopedGObject<GSocket> socket{g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, &error)};

    if (!socket) {
        g_warning("Failed to setup socket for incoming source connections: %s", error->message);
        g_error_free(error);
        return false;
    }

    auto addr = g_inet_socket_address_new_from_string(address.c_str(), port);
    if (!g_socket_bind(socket.get(), addr, TRUE, &error)) {
        g_warning("Failed to setup socket for incoming source connections: %s", error->message);
        g_error_free(error);
        return false;
    }

    if (!g_socket_listen(socket.get(), &error)) {
        g_warning("Failed start listening for incoming connections: %s", error->message);
        g_error_free(error);
        return false;
    }

    auto source = g_socket_create_source(socket.get(), G_IO_IN, nullptr);
    if (!source) {
        g_warning("Failed to setup listener for incoming connections");
        return false;
    }

    g_source_set_callback(source, (GSourceFunc) &MiracastSource::OnNewConnection, this, nullptr);
    socket_source_ = g_source_attach(source, nullptr);
    if (socket_source_ == 0) {
        g_warning("Failed to attach source to mainloop");
        g_source_unref(source);
        return false;
    }

    g_source_unref(source);

    g_info("Successfully setup source on %s:%d and awaiting incoming connection requests",
           address.c_str(), port);

    socket_.swap(socket);

    return true;
}

void MiracastSource::Release() {    
    if (socket_source_ > 0) {
        g_source_remove(socket_source_);
        socket_source_ = 0;
    }

    if (active_sink_) {
        delete active_sink_;
        active_sink_ = nullptr;
    }
}

gboolean MiracastSource::OnNewConnection(GSocket *socket, GIOCondition  cond, gpointer user_data) {
    auto inst = static_cast<MiracastSource*>(user_data);

    GError *error = nullptr;
    auto client_socket = g_socket_accept(inst->socket_.get(), NULL, &error);
    if (!client_socket) {
        g_warning("Failed to accept incoming connection: %s", error->message);
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

    inst->active_sink_ = new MiracastSourceClient(inst, client_socket);

    return FALSE;
}

void MiracastSource::OnConnectionClosed() {
    if (delegate_)
        delegate_->OnClientDisconnected();
}
} // namespace mcs
