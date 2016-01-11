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

#include <mcs/logger.h>
#include <mcs/networkutils.h>

#include <w11tng/dhcpserver.h>

namespace w11tng {
DhcpServer::DhcpServer(Delegate *delegate, const std::string &interface_name) :
    interface_name_(interface_name) {
}

DhcpServer::~DhcpServer() {
    Stop();
}

mcs::IpV4Address DhcpServer::LocalAddress() const {
    // FIXME this should be stored somewhere else
    return mcs::IpV4Address::from_string("192.168.7.1");
}

bool DhcpServer::Start() {
    return true;
}

void DhcpServer::Stop() {
}
}
