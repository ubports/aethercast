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

#include <gio/gio.h>

#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

#include <mcs/logger.h>
#include <mcs/networkutils.h>
#include <mcs/keep_alive.h>
#include <mcs/scoped_gobject.h>

#include <w11tng/dhcpclient.h>
#include <w11tng/config.h>

namespace w11tng {

DhcpClient::Ptr DhcpClient::Create(Delegate *delegate, const std::string &interface_name) {
    return std::shared_ptr<DhcpClient>(new DhcpClient(delegate, interface_name));
}

DhcpClient::DhcpClient(Delegate *delegate, const std::string &interface_name) :
    delegate_(delegate),
    interface_name_(interface_name) {
}

DhcpClient::~DhcpClient() {
    Stop();
}

mcs::IpV4Address DhcpClient::LocalAddress() const {
    return local_address_;
}

bool DhcpClient::Start() {
    if (pid_ > 0)
        return true;

    auto argv = g_ptr_array_new();

    g_ptr_array_add(argv, (gpointer) kDhcpClientPath);

    // Disable background on lease (let dhcpd not fork)
    g_ptr_array_add(argv, (gpointer) "-d");
    // Don't be verbose
    g_ptr_array_add(argv, (gpointer) "-q");

    g_ptr_array_add(argv, (gpointer) "-sf");

    auto override_helper_path = mcs::Utils::GetEnvValue("AETHERCAST_W11TNG_DHCP_HELPER");
    g_ptr_array_add(argv, (gpointer) (override_helper_path.length() > 0 ? override_helper_path.c_str() : kDhcpHelperPath));

    // We only want dhclient to operate on the P2P interface an no other
    g_ptr_array_add(argv, (gpointer) interface_name_.c_str());

    g_ptr_array_add(argv, nullptr);

    auto cmdline = g_strjoinv(" ", reinterpret_cast<gchar**>(argv->pdata));
    MCS_DEBUG("Running dhclient with: %s", cmdline);
    g_free(cmdline);

    GError *error = nullptr;
    if (!g_spawn_async(nullptr, reinterpret_cast<gchar**>(argv->pdata), nullptr,
                       GSpawnFlags(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL),
                       nullptr, nullptr, &pid_, &error)) {
        MCS_ERROR("Failed to spawn DHCP client: %s", error->message);
        g_error_free(error);
        g_ptr_array_free(argv, TRUE);
        return false;
    }

    g_child_watch_add_full(0, pid_, [](GPid pid, gint status, gpointer user_data) {
        auto inst = static_cast<mcs::WeakKeepAlive<DhcpClient>*>(user_data)->GetInstance().lock();

        if (!WIFEXITED(status))
            MCS_WARNING("DHCP client (pid %d) exited with status %d", pid, status);
        else
            MCS_DEBUG("DHCP client (pid %d) successfully terminated", pid);

        inst->pid_ = -1;

    }, new mcs::WeakKeepAlive<DhcpClient>(shared_from_this()), [](gpointer data) { delete static_cast<mcs::WeakKeepAlive<DhcpClient>*>(data); });

    return true;
}

void DhcpClient::Stop() {
    if (pid_ <= 0)
        return;

    ::kill(pid_, SIGTERM);
    g_spawn_close_pid(pid_);

    pid_ = 0;
}
}
