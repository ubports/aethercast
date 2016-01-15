/*
 * lxc: linux Container library
 *
 * (C) Copyright IBM Corp. 2007, 2008
 *
 * Authors:
 * Daniel Lezcano <daniel.lezcano at free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define _GNU_SOURCE
#include <stdio.h>
#undef _GNU_SOURCe
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
#include <ifaddrs.h>

#include "nl.h"

#ifndef VETH_INFO_PEER
# define VETH_INFO_PEER 1
#endif

int lxc_veth_create(const char *name1, const char *name2)
{
    struct nl_handler nlh;
    struct nlmsg *nlmsg = NULL, *answer = NULL;
    struct ifinfomsg *ifi;
    struct rtattr *nest1, *nest2, *nest3;
    int len, err;

    printf("name1 %s name2 %s\n", name1, name2);

    err = netlink_open(&nlh, NETLINK_ROUTE);
    if (err)
        return err;

    err = -EINVAL;
    len = strlen(name1);
    if (len == 1 || len >= IFNAMSIZ)
        goto out;

    len = strlen(name2);
    if (len == 1 || len >= IFNAMSIZ)
        goto out;

    err = -ENOMEM;
    nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
    if (!nlmsg)
        goto out;

    answer = nlmsg_alloc_reserve(NLMSG_GOOD_SIZE);
    if (!answer)
        goto out;

    nlmsg->nlmsghdr->nlmsg_flags =
        NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL|NLM_F_ACK;
    nlmsg->nlmsghdr->nlmsg_type = RTM_NEWLINK;

    ifi = nlmsg_reserve(nlmsg, sizeof(struct ifinfomsg));
    if (!ifi)
        goto out;
    ifi->ifi_family = AF_UNSPEC;

    err = -EINVAL;
    nest1 = nla_begin_nested(nlmsg, IFLA_LINKINFO);
    if (!nest1)
        goto out;

    if (nla_put_string(nlmsg, IFLA_INFO_KIND, "veth"))
        goto out;

    nest2 = nla_begin_nested(nlmsg, IFLA_INFO_DATA);
    if (!nest2)
        goto out;

    nest3 = nla_begin_nested(nlmsg, VETH_INFO_PEER);
    if (!nest3)
        goto out;

    ifi = nlmsg_reserve(nlmsg, sizeof(struct ifinfomsg));
    if (!ifi) {
        err = -ENOMEM;
        goto out;
    }

    if (nla_put_string(nlmsg, IFLA_IFNAME, name2))
        goto out;

    nla_end_nested(nlmsg, nest3);

    nla_end_nested(nlmsg, nest2);

    nla_end_nested(nlmsg, nest1);

    if (nla_put_string(nlmsg, IFLA_IFNAME, name1))
        goto out;

    err = netlink_transaction(&nlh, nlmsg, answer);
out:
    netlink_close(&nlh);
    nlmsg_free(answer);
    nlmsg_free(nlmsg);
    return err;
}

int lxc_netdev_delete_by_index(int ifindex)
{
    struct nl_handler nlh;
    struct nlmsg *nlmsg = NULL, *answer = NULL;
    struct ifinfomsg *ifi;
    int err;

    err = netlink_open(&nlh, NETLINK_ROUTE);
    if (err)
        return err;

    err = -ENOMEM;
    nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
    if (!nlmsg)
        goto out;

    answer = nlmsg_alloc_reserve(NLMSG_GOOD_SIZE);
    if (!answer)
        goto out;

    nlmsg->nlmsghdr->nlmsg_flags = NLM_F_ACK|NLM_F_REQUEST;
    nlmsg->nlmsghdr->nlmsg_type = RTM_DELLINK;

    ifi = nlmsg_reserve(nlmsg, sizeof(struct ifinfomsg));
    if (!ifi)
        goto out;
    ifi->ifi_family = AF_UNSPEC;
    ifi->ifi_index = ifindex;

    err = netlink_transaction(&nlh, nlmsg, answer);
out:
    netlink_close(&nlh);
    nlmsg_free(answer);
    nlmsg_free(nlmsg);
    return err;
}

int lxc_netdev_delete_by_name(const char *name)
{
    int index;

    index = if_nametoindex(name);
    if (!index)
        return -EINVAL;

    return lxc_netdev_delete_by_index(index);
}

static const char padchar[] =
"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

char *lxc_mkifname(char *tmplate)
{
    char *name = NULL;
    int i = 0;
    FILE *urandom;
    unsigned int seed;
    struct ifaddrs *ifaddr, *ifa;
    int ifexists = 0;

    /* Get all the network interfaces */
    getifaddrs(&ifaddr);

    /* Initialize the random number generator */
    urandom = fopen ("/dev/urandom", "r");
    if (urandom != NULL) {
        if (fread (&seed, sizeof(seed), 1, urandom) <= 0)
            seed = time(0);
        fclose(urandom);
    }
    else
        seed = time(0);

#ifndef HAVE_RAND_R
    srand(seed);
#endif

    /* Generate random names until we find one that doesn't exist */
    while(1) {
        ifexists = 0;
        name = strdup(tmplate);

        if (name == NULL)
            return NULL;

        for (i = 0; i < strlen(name); i++) {
            if (name[i] == 'X') {
#ifdef HAVE_RAND_R
                name[i] = padchar[rand_r(&seed) % (strlen(padchar) - 1)];
#else
                name[i] = padchar[rand() % (strlen(padchar) - 1)];
#endif
            }
        }

        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (strcmp(ifa->ifa_name, name) == 0) {
                ifexists = 1;
                break;
            }
        }

        if (ifexists == 0)
            break;

        free(name);
    }

    freeifaddrs(ifaddr);
    return name;
}

static int netdev_set_flag(const char *name, int flag)
{
    struct nl_handler nlh;
    struct nlmsg *nlmsg = NULL, *answer = NULL;
    struct ifinfomsg *ifi;
    int index, len, err;

    err = netlink_open(&nlh, NETLINK_ROUTE);
    if (err)
        return err;

    err = -EINVAL;
    len = strlen(name);
    if (len == 1 || len >= IFNAMSIZ)
        goto out;

    err = -ENOMEM;
    nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
    if (!nlmsg)
        goto out;

    answer = nlmsg_alloc_reserve(NLMSG_GOOD_SIZE);
    if (!answer)
        goto out;

    err = -EINVAL;
    index = if_nametoindex(name);
    if (!index)
        goto out;

    nlmsg->nlmsghdr->nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK;
    nlmsg->nlmsghdr->nlmsg_type = RTM_NEWLINK;

    ifi = nlmsg_reserve(nlmsg, sizeof(struct ifinfomsg));
    if (!ifi) {
        err = -ENOMEM;
        goto out;
    }
    ifi->ifi_family = AF_UNSPEC;
    ifi->ifi_index = index;
    ifi->ifi_change |= IFF_UP;
    ifi->ifi_flags |= flag;

    err = netlink_transaction(&nlh, nlmsg, answer);
out:
    netlink_close(&nlh);
    nlmsg_free(nlmsg);
    nlmsg_free(answer);
    return err;
}

int lxc_netdev_up(const char *name)
{
    return netdev_set_flag(name, IFF_UP);
}

int lxc_netdev_down(const char *name)
{
    return netdev_set_flag(name, 0);
}
