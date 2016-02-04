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

#include <sys/ioctl.h>
#include <net/if.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <memory.h>

#include <glib.h>

#include "logger.h"
#include "networkutils.h"
#include "logger.h"

#define NLMSG_TAIL(nmsg)				\
    ((struct rtattr *) (((uint8_t*) (nmsg)) +	\
    NLMSG_ALIGN((nmsg)->nlmsg_len)))

int __rtnl_addattr_l(struct nlmsghdr *n, size_t max_length,
                int type, const void *data, size_t data_length)
{
    size_t length;
    struct rtattr *rta;

    length = RTA_LENGTH(data_length);

    if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(length) > max_length)
        return -E2BIG;

    rta = NLMSG_TAIL(n);
    rta->rta_type = type;
    rta->rta_len = length;
    memcpy(RTA_DATA(rta), data, data_length);
    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(length);

    return 0;
}

namespace mcs {
int NetworkUtils::ModifyInterfaceAddress(int cmd, int flags,
                int index, int family,
                const char *address,
                const char *peer,
                unsigned char prefixlen,
                const char *broadcast)
{
    uint8_t request[NLMSG_ALIGN(sizeof(struct nlmsghdr)) +
            NLMSG_ALIGN(sizeof(struct ifaddrmsg)) +
            RTA_LENGTH(sizeof(struct in6_addr)) +
            RTA_LENGTH(sizeof(struct in6_addr))];

    struct nlmsghdr *header;
    struct sockaddr_nl nl_addr;
    struct ifaddrmsg *ifaddrmsg;
    struct in6_addr ipv6_addr;
    struct in_addr ipv4_addr, ipv4_dest, ipv4_bcast;
    int sk, err;

    if (!address)
        return -EINVAL;

    if (family != AF_INET && family != AF_INET6)
        return -EINVAL;

    memset(&request, 0, sizeof(request));

    header = (struct nlmsghdr *)request;
    header->nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    header->nlmsg_type = cmd;
    header->nlmsg_flags = NLM_F_REQUEST | flags;
    header->nlmsg_seq = 1;

    ifaddrmsg = (struct ifaddrmsg *) NLMSG_DATA(header);
    ifaddrmsg->ifa_family = family;
    ifaddrmsg->ifa_prefixlen = prefixlen;
    ifaddrmsg->ifa_flags = IFA_F_PERMANENT;
    ifaddrmsg->ifa_scope = RT_SCOPE_UNIVERSE;
    ifaddrmsg->ifa_index = index;

    if (family == AF_INET) {
        if (inet_pton(AF_INET, address, &ipv4_addr) < 1)
            return -1;

        if (broadcast)
            inet_pton(AF_INET, broadcast, &ipv4_bcast);
        else
            ipv4_bcast.s_addr = ipv4_addr.s_addr |
                htonl(0xfffffffflu >> prefixlen);

        if (peer) {
            if (inet_pton(AF_INET, peer, &ipv4_dest) < 1)
                return -1;

            err = __rtnl_addattr_l(header,
                            sizeof(request),
                            IFA_ADDRESS,
                            &ipv4_dest,
                            sizeof(ipv4_dest));
            if (err < 0)
                return err;
        }

        err = __rtnl_addattr_l(header,
                        sizeof(request),
                        IFA_LOCAL,
                        &ipv4_addr,
                        sizeof(ipv4_addr));
        if (err < 0)
            return err;

        err = __rtnl_addattr_l(header,
                        sizeof(request),
                        IFA_BROADCAST,
                        &ipv4_bcast,
                        sizeof(ipv4_bcast));
        if (err < 0)
            return err;

    } else if (family == AF_INET6) {
        if (inet_pton(AF_INET6, address, &ipv6_addr) < 1)
            return -1;

        err = __rtnl_addattr_l(header,
                        sizeof(request),
                        IFA_LOCAL,
                        &ipv6_addr,
                        sizeof(ipv6_addr));
        if (err < 0)
            return err;
    }

    sk = socket(AF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_ROUTE);
    if (sk < 0)
        return -errno;

    memset(&nl_addr, 0, sizeof(nl_addr));
    nl_addr.nl_family = AF_NETLINK;

    if ((err = sendto(sk, request, header->nlmsg_len, 0,
            (struct sockaddr *) &nl_addr, sizeof(nl_addr))) < 0)
        goto done;

    err = 0;

done:
    close(sk);

    return err;
}

int NetworkUtils::RetrieveInterfaceIndex(const char *name)
{
    struct ifreq ifr;
    int sk, err;

    if (name == nullptr)
        return -1;

    sk = socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (sk < 0)
        return -1;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name) - 1);

    err = ioctl(sk, SIOCGIFINDEX, &ifr);

    close(sk);

    if (err < 0)
        return -1;

    return ifr.ifr_ifindex;;
}

std::string NetworkUtils::RetrieveInterfaceName(int index) {
    if (index < 0)
        return "";

    auto sock = ::socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (sock < 0)
        return "";

    struct ifreq ifr = { };
    ifr.ifr_ifindex = index;

    auto err = ::ioctl(sock, SIOCGIFNAME, &ifr);

    ::close(sock);

    if (err < 0)
        return "";

    return std::string(ifr.ifr_name);
}

int NetworkUtils::ResetInterface(int index)
{
    struct ifreq ifr, addr_ifr;
    struct sockaddr_in *addr;
    int sk, err;

    sk = socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (sk < 0)
        return -errno;

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_ifindex = index;

    if (ioctl(sk, SIOCGIFNAME, &ifr) < 0) {
        err = -errno;
        goto done;
    }

    if (ioctl(sk, SIOCGIFFLAGS, &ifr) < 0) {
        err = -errno;
        goto done;
    }

    memset(&addr_ifr, 0, sizeof(addr_ifr));
    memcpy(&addr_ifr.ifr_name, &ifr.ifr_name, sizeof(ifr.ifr_name) - 1);
    addr = (struct sockaddr_in *)&addr_ifr.ifr_addr;
    addr->sin_family = AF_INET;
    if (ioctl(sk, SIOCSIFADDR, &addr_ifr) < 0)
        WARNING("Could not clear IPv4 address of interface with index %d", index);

done:
    close(sk);

    return err;
}

int NetworkUtils::BytesAvailableToRead(int fd) {
    int nbytes = 0;
    // gives shorter than true amounts on Unix domain sockets.
    int64_t available = 0;
    if (ioctl(fd, FIONREAD, (char *) &nbytes) >= 0)
        available = (int64_t) nbytes;
    return available;
}
} // namespace mcs
