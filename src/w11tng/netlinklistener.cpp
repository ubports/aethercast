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
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <mcs/logger.h>
#include <mcs/keep_alive.h>
#include <mcs/networkutils.h>

#include "netlinklistener.h"

namespace w11tng {
NetlinkListener::Ptr NetlinkListener::Create(const std::weak_ptr<Delegate> &delegate) {
    return std::shared_ptr<NetlinkListener>(new NetlinkListener(delegate))->FinalizeConstruction();
}

NetlinkListener::Ptr NetlinkListener::FinalizeConstruction() {
    auto sp = shared_from_this();

    auto fd = ::socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
        MCS_ERROR("Could not connect with netlink");
        return sp;
    }

    struct sockaddr_nl addr = {};
    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

    if (::bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        MCS_ERROR("Failed to bind netlink socket");
        ::close(fd);
        return sp;
    }

    channel_ = g_io_channel_unix_new(fd);
    g_io_channel_set_encoding(channel_, nullptr, nullptr);
    g_io_channel_set_buffered(channel_, FALSE);
    g_io_channel_set_close_on_unref(channel_, TRUE);

    g_io_add_watch_full(channel_, 0, GIOCondition(G_IO_IN | G_IO_NVAL | G_IO_ERR | G_IO_HUP),
                        NetlinkListener::OnDataAvailable,
                        new mcs::WeakKeepAlive<NetlinkListener>(sp),
                        [](gpointer data) { delete static_cast<mcs::WeakKeepAlive<NetlinkListener>*>(data); });

    return sp;
}

NetlinkListener::NetlinkListener(const std::weak_ptr<Delegate> &delegate) :
    delegate_(delegate),
    channel_(nullptr),
    interface_index_filter_(0) {
}

NetlinkListener::~NetlinkListener() {
    if (channel_)
        g_io_channel_unref(channel_);
}

void NetlinkListener::SetInterfaceFilter(const std::string &interface_name) {
    interface_index_filter_ = mcs::NetworkUtils::RetrieveInterfaceIndex(interface_name.c_str());
}

std::string TypeToString(uint16_t type) {
    switch (type) {
    case NLMSG_NOOP:
        return "NOOP";
    case NLMSG_ERROR:
        return "ERROR";
    case NLMSG_DONE:
        return "DONE";
    case NLMSG_OVERRUN:
        return "OVERRUN";
    case RTM_GETLINK:
        return "GETLINK";
    case RTM_NEWLINK:
        return "NEWLINK";
    case RTM_DELLINK:
        return "DELLINK";
    case RTM_GETADDR:
        return "GETADDR";
    case RTM_NEWADDR:
        return "NEWADDR";
    case RTM_DELADDR:
        return "DELADDR";
    case RTM_GETROUTE:
        return "GETROUTE";
    case RTM_NEWROUTE:
        return "NEWROUTE";
    case RTM_DELROUTE:
        return "DELROUTE";
    case RTM_NEWNDUSEROPT:
        return "NEWNDUSEROPT";
    default:
        break;
    }
    return "unknown";
}

void NetlinkListener::ProcessNewAddress(struct nlmsghdr *hdr) {
    auto msg = (struct ifaddrmsg*) NLMSG_DATA(hdr);
    auto bytes = IFA_PAYLOAD(hdr);

    if (interface_index_filter_ > 0 && msg->ifa_index != interface_index_filter_)
        return;

    MCS_DEBUG("family %d index %d", (unsigned int) msg->ifa_family, msg->ifa_index);

    for (auto attr = IFA_RTA(msg); RTA_OK(attr, bytes); attr = RTA_NEXT(attr, bytes)) {
        switch (attr->rta_type) {
        case IFA_ADDRESS:
            auto len = static_cast<int>(RTA_PAYLOAD(attr));

            if (msg->ifa_family == AF_INET && len == sizeof(struct in_addr)) {
                struct in_addr addr;
                addr = *((struct in_addr*) RTA_DATA(attr));
                MCS_DEBUG("  attr address (len %d) %s", len, inet_ntoa(addr));

                if (auto sp = delegate_.lock())
                    sp->OnInterfaceAddressChanged(mcs::NetworkUtils::RetrieveInterfaceName(msg->ifa_index),
                                                  std::string(inet_ntoa(addr)));
            }
            break;
        }
    }
}

gboolean NetlinkListener::OnDataAvailable(GIOChannel *channel, GIOCondition condition, gpointer user_data) {
    auto thiz = static_cast<mcs::WeakKeepAlive<NetlinkListener>*>(user_data)->GetInstance().lock();
    if (not thiz)
        return FALSE;

    MCS_DEBUG("");

    auto fd = g_io_channel_unix_get_fd(thiz->channel_);

    char buf[4096];
    struct iovec iov = { buf, sizeof buf };
    struct sockaddr_nl snl;
    struct msghdr msg = { static_cast<void*>(&snl), sizeof(snl), &iov, 1, nullptr, 0, 0 };

    auto ret = ::recvmsg(fd, &msg, 0);
    if (ret < 0)
        return TRUE;

    auto len = ret;
    void *ptr = &buf[0];
    while (len > 0) {
        auto hdr = (struct nlmsghdr*) ptr;

        if (!NLMSG_OK(hdr, len))
            break;

        MCS_DEBUG("%s len %d type %d flags %d seq %d pid %d",
                  TypeToString(hdr->nlmsg_type),
                  hdr->nlmsg_len,
                  hdr->nlmsg_type,
                  hdr->nlmsg_flags,
                  hdr->nlmsg_seq,
                  hdr->nlmsg_pid);

        switch (hdr->nlmsg_type) {
        case RTM_NEWADDR:
            thiz->ProcessNewAddress(hdr);
            break;
        default:
            break;
        }

        len -= hdr->nlmsg_len;
        ptr += hdr->nlmsg_len;
    }

    return TRUE;
}

} // namespace w11tng
