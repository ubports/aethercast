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

#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <sys/prctl.h>

#include <boost/filesystem.hpp>

#include <ac/logger.h>
#include <ac/networkutils.h>
#include <ac/keep_alive.h>
#include <ac/scoped_gobject.h>

#include <ac/config.h>
#include <w11tng/config.h>

#include "dhcpclient.h"
#include "dhcpleaseparser.h"

namespace w11tng {

DhcpClient::Ptr DhcpClient::Create(const std::weak_ptr<Delegate> &delegate, const std::string &interface_name) {
    auto sp = std::shared_ptr<DhcpClient>(new DhcpClient(delegate, interface_name));
    sp->Start();
    return sp;
}

DhcpClient::DhcpClient(const std::weak_ptr<Delegate> &delegate, const std::string &interface_name) :
    delegate_(delegate),
    interface_name_(interface_name) {

    lease_file_path_ = ac::Utils::Sprintf("%s/dhclient-%s-%s.leases",
                                    ac::kRuntimePath,
                                    boost::filesystem::unique_path().string(),
                                    interface_name_);
}

DhcpClient::~DhcpClient() {
    ::unlink(lease_file_path_.c_str());
}

ac::IpV4Address DhcpClient::LocalAddress() const {
    return local_address_;
}

ac::IpV4Address DhcpClient::RemoteAddress() const {
    return remote_address_;
}

void DhcpClient::Start() {
    if (!ac::Utils::CreateFile(lease_file_path_)) {
        AC_ERROR("Failed to create database for DHCP leases at %s",
                  lease_file_path_);
        return;
    }

    monitor_ = FileMonitor::Create(lease_file_path_, shared_from_this());

    std::vector<std::string> argv = {
        // Disable background on lease (let dhclient not fork)
        "-d",
        // Be verbose and not quiet on startup
        "-v",
        // Force us to be IPv4 no matter what the default is
        "-4",
        // Use the temporary lease file we used above to not interfere
        // with any other parts in the system which are using dhclient
        // as well. We also want a fresh lease file on every start.
        "-lf", lease_file_path_,
        // We only want dhclient to operate on the P2P interface an no other
        interface_name_
    };

    executor_ = ProcessExecutor::Create(kDhcpClientPath, argv, shared_from_this());
}

void DhcpClient::OnProcessTerminated() {
    if (auto sp = delegate_.lock())
        sp->OnDhcpTerminated();
}

void DhcpClient::OnFileChanged(const std::string &path) {
    auto leases = DhcpLeaseParser::FromFile(path);
    if (leases.size() != 1)
        return;

    auto actual_lease = leases[0];

    local_address_ = actual_lease.FixedAddress();
    remote_address_ = actual_lease.Gateway();

    if (auto sp = delegate_.lock())
        sp->OnDhcpAddressAssigned(local_address_, remote_address_);
}
} // namespace w11tng
