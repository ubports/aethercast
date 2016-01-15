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

#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

#include <mcs/logger.h>
#include <mcs/networkutils.h>
#include <mcs/keep_alive.h>

#include <w11tng/dhcpserver.h>
#include <w11tng/config.h>

namespace w11tng {

DhcpServer::Ptr DhcpServer::Create(Delegate *delegate, const std::string &interface_name) {
    return std::shared_ptr<DhcpServer>(new DhcpServer(delegate, interface_name));
}

DhcpServer::DhcpServer(Delegate *delegate, const std::string &interface_name) :
    interface_name_(interface_name),
    pid_(-1) {
}

DhcpServer::~DhcpServer() {
    Stop();
}

mcs::IpV4Address DhcpServer::LocalAddress() const {
    // FIXME this should be stored somewhere else
    return mcs::IpV4Address::from_string("192.168.7.1");
}

bool DhcpServer::Start() {
    if (pid_ > 0)
        return true;

    auto argv = g_ptr_array_new();

    g_ptr_array_add(argv, (gpointer) kDhcpServerPath);

    // Disable background on lease (let dhcpd not fork)
    g_ptr_array_add(argv, (gpointer) "-f");

    // WiFi Direct is only specified for IPv4 so we insist on not using
    // any IPv6 support.
    g_ptr_array_add(argv, (gpointer) "-4");

    // We only want dhcpd to listen on the P2P interface an no other
    // which it would do if we don't supply an interface here.
    g_ptr_array_add(argv, (gpointer) interface_name_.c_str());

    g_ptr_array_add(argv, nullptr);

    auto cmdline = g_strjoinv(" ", reinterpret_cast<gchar**>(argv->pdata));
    MCS_DEBUG("Running dhcp-server with: %s", cmdline);
    g_free(cmdline);

    GError *error = nullptr;
    if (!g_spawn_async(nullptr, reinterpret_cast<gchar**>(argv->pdata), nullptr,
                       GSpawnFlags(G_SPAWN_DO_NOT_REAP_CHILD),
                       nullptr, nullptr, &pid_, &error)) {
        MCS_ERROR("Failed to spawn DHCP server: %s", error->message);
        g_error_free(error);
        g_ptr_array_free(argv, TRUE);
        return false;
    }

    g_child_watch_add_full(0, pid_, [](GPid pid, gint status, gpointer user_data) {
        auto inst = static_cast<mcs::WeakKeepAlive<DhcpServer>*>(user_data)->GetInstance().lock();

        if (!WIFEXITED(status))
            MCS_WARNING("DHCP server (pid %d) exited with status %d", pid, status);
        else
            MCS_DEBUG("DHCP server successfully terminated");

        inst->pid_ = -1;

    }, new mcs::WeakKeepAlive<DhcpServer>(shared_from_this()), [](gpointer data) { delete static_cast<mcs::WeakKeepAlive<DhcpServer>*>(data); });

    return true;
}

void DhcpServer::Stop() {
    if (pid_ <= 0)
        return;

    ::kill(pid_, SIGTERM);
    g_spawn_close_pid(pid_);

    pid_ = 0;
}
}
