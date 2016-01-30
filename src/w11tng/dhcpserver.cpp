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
#include <sys/prctl.h>

#include <boost/filesystem.hpp>

#include <mcs/config.h>
#include <mcs/logger.h>
#include <mcs/networkutils.h>
#include <mcs/keep_alive.h>

#include <w11tng/config.h>

#include "dhcpserver.h"
#include "dhcpleaseparser.h"

namespace w11tng {

DhcpServer::Ptr DhcpServer::Create(const std::weak_ptr<Delegate> &delegate, const std::string &interface_name) {
    auto sp = std::shared_ptr<DhcpServer>(new DhcpServer(delegate, interface_name));
    sp->Start();
    return sp;
}

DhcpServer::DhcpServer(const std::weak_ptr<Delegate> &delegate, const std::string &interface_name) :
    delegate_(delegate),
    interface_name_(interface_name) {

    lease_file_path_ = mcs::Utils::Sprintf("%s/dhcpd-%s-%s.leases",
                                           mcs::kRuntimePath,
                                           boost::filesystem::unique_path().string(),
                                           interface_name);

    pid_file_path_ = mcs::Utils::Sprintf("%s/dhcpd-%s.pid",
                                         mcs::kRuntimePath,
                                         interface_name_);
}

DhcpServer::~DhcpServer() {
    ::unlink(lease_file_path_.c_str());
    ::unlink(pid_file_path_.c_str());
}

void DhcpServer::Start() {
    if (boost::filesystem::is_regular(pid_file_path_)) {
        MCS_ERROR("DHCP server already running");
        return;
    }

    if (!mcs::Utils::CreateFile(lease_file_path_)) {
        MCS_ERROR("Failed to create database for DHCP leases at %s",
                  lease_file_path_);
        return;
    }

    monitor_ = FileMonitor::Create(lease_file_path_, shared_from_this());

    // FIXME store those defaults somewhere else
    const char *address = "192.168.7.1";
    const char *broadcast = "192.168.7.255";
    unsigned char prefixlen = 24;

    local_address_ = mcs::IpV4Address::from_string(address);

    auto interface_index = mcs::NetworkUtils::RetrieveInterfaceIndex(interface_name_.c_str());
    if (interface_index < 0)
        MCS_ERROR("Failed to determine index of network interface: %s", interface_name_);

    if (mcs::NetworkUtils::ModifyInterfaceAddress(RTM_NEWADDR, NLM_F_REPLACE | NLM_F_ACK, interface_index,
                                    AF_INET, address,
                                    NULL, prefixlen, broadcast) < 0) {
        MCS_ERROR("Failed to assign network address for %s", interface_name_);
        return;
    }

    MCS_DEBUG("Assigned network address %s", address);

    std::vector<std::string> argv = {
        "-f", "-4",
        "-d",
        "-cf", "/etc/aethercast/dhcpd.conf",
        "-pf", pid_file_path_,
        "-lf", lease_file_path_,
        interface_name_
    };

    executor_ = ProcessExecutor::Create(kDhcpServerPath, argv, shared_from_this());
}

void DhcpServer::OnProcessTerminated() {
}

void DhcpServer::OnFileChanged(const std::string &path) {
    auto leases = DhcpLeaseParser::FromFile(path);
    if (leases.size() != 1)
        return;

    auto actual_lease = leases[0];

    if (auto sp = delegate_.lock())
        sp->OnAddressAssigned(local_address_, actual_lease.FixedAddress());
}

mcs::IpV4Address DhcpServer::LocalAddress() const {
    return local_address_;
}

}
