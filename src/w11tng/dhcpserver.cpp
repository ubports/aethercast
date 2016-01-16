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

#include <boost/filesystem.hpp>

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
    pid_(-1),
    process_watch_(0) {
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

    lease_file_path_ = mcs::Utils::Sprintf("%s/aethercast-dhcp-leases-%s",
                                    boost::filesystem::temp_directory_path().string(),
                                    boost::filesystem::unique_path().string());
    if (!mcs::Utils::CreateFile(lease_file_path_)) {
        MCS_ERROR("Failed to create database for DHCP leases at %s",
                  lease_file_path_);
        return false;
    }

    // FIXME store those defaults somewhere else
    const char *address = "192.168.7.1";
    const char *broadcast = "192.168.7.255";
    unsigned char prefixlen = 24;

    auto interface_index = mcs::NetworkUtils::RetrieveInterfaceIndex(interface_name_.c_str());
    if (interface_index < 0)
        MCS_ERROR("Failed to determine index of network interface: %s", interface_name_);

    if (mcs::NetworkUtils::ModifyInterfaceAddress(RTM_NEWADDR, NLM_F_REPLACE | NLM_F_ACK, interface_index,
                                    AF_INET, address,
                                    NULL, prefixlen, broadcast) < 0) {
        MCS_ERROR("Failed to assign network address for %s", interface_name_);
        return false;
    }

    MCS_DEBUG("Assigned network address %s", address);

    auto argv = g_ptr_array_new();

    g_ptr_array_add(argv, (gpointer) kDhcpServerPath);

    // Disable background on lease (let dhcpd not fork)
    g_ptr_array_add(argv, (gpointer) "-f");

    // WiFi Direct is only specified for IPv4 so we insist on not using
    // any IPv6 support.
    g_ptr_array_add(argv, (gpointer) "-4");

    g_ptr_array_add(argv, (gpointer) "-cf");
    g_ptr_array_add(argv, (gpointer) "/home/simon/Work/ubuntu/p2p/ac-wpa-dbus/conf/dhcpd.conf");

    g_ptr_array_add(argv, (gpointer) "-lf");
    g_ptr_array_add(argv, (gpointer) lease_file_path_.c_str());

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
                       [](gpointer user_data) { }, nullptr,
                       &pid_, &error)) {

        MCS_ERROR("Failed to spawn DHCP server: %s", error->message);
        g_error_free(error);
        g_ptr_array_free(argv, TRUE);
        return false;
    }

    process_watch_ = g_child_watch_add_full(0, pid_, [](GPid pid, gint status, gpointer user_data) {
        auto inst = static_cast<mcs::WeakKeepAlive<DhcpServer>*>(user_data)->GetInstance().lock();

        if (!WIFEXITED(status))
            MCS_WARNING("DHCP server (pid %d) exited with status %d", pid, status);
        else
            MCS_DEBUG("DHCP server successfully terminated");

        if (not inst)
            return;

        inst->pid_ = -1;
    }, new mcs::WeakKeepAlive<DhcpServer>(shared_from_this()), [](gpointer data) { delete static_cast<mcs::WeakKeepAlive<DhcpServer>*>(data); });

    return true;
}

void DhcpServer::Stop() {
    if (pid_ <= 0)
        return;

    ::kill(pid_, SIGTERM);
    g_spawn_close_pid(pid_);

    pid_ = -1;

    ::unlink(lease_file_path_.c_str());

    if (process_watch_ > 0)
        g_source_remove(process_watch_);
}
}
