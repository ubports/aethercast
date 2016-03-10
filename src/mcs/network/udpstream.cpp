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

#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <memory.h>
#include <errno.h>
#include <error.h>
#include <stdlib.h>

#include "mcs/logger.h"

#include "mcs/network/udpstream.h"

namespace {
static constexpr unsigned int kUdpTxBufferSize = 256 * 1024;

static mcs::network::Port PickRandomRTPPort() {
    // Pick an even integer in range [1024, 65534)
    static const size_t range = (65534 - 1024) / 2;
    return (mcs::network::Port)(((float)(range + 1) * rand()) / RAND_MAX) * 2 + 1024;
}

static int MakeSocketNonBlocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0)
        flags = 0;
    int res = fcntl(socket, F_SETFL, flags | O_NONBLOCK);
    if (res < 0)
        return -errno;
    return 0;
}

}

namespace mcs {
namespace network {

UdpStream::UdpStream(const IpV4Address &address, const Port &port) :
    socket_(0),
    local_port_(PickRandomRTPPort()) {

    MCS_DEBUG("Connected with remote on %s:%d", address, port);

    socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ < 0)
        throw new std::system_error{errno, std::system_category(), "Failed to create socket"};

    int value = kUdpTxBufferSize;
    if (::setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &value, sizeof(value)) < 0)
        throw new std::system_error{errno, std::system_category(), "Failed to set socket transmit buffer size"};

    if (::MakeSocketNonBlocking(socket_) < 0)
        throw new std::runtime_error("Failed to make socket non-blocking");

    struct sockaddr_in addr;
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(local_port_);

    if (::bind(socket_, (const struct sockaddr *) &addr, sizeof(addr)) < 0)
        throw new std::system_error{errno, std::system_category(), "Failed to bind socket to address"};

    struct sockaddr_in remote_addr;
    memset(remote_addr.sin_zero, 0, sizeof(remote_addr.sin_zero));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);

    struct hostent *ent = gethostbyname(address.to_string().c_str());
    if (!ent)
        throw new std::runtime_error("Failed to resolve remote address");

    remote_addr.sin_addr.s_addr = *(in_addr_t*) ent->h_addr;

    if (::connect(socket_, (const struct sockaddr*) &remote_addr, sizeof(remote_addr)) < 0)
        throw new std::system_error{errno, std::system_category(), "Failed to connect with remote"};
}

UdpStream::~UdpStream() {
    if (socket_ > 0)
        ::close(socket_);
}

bool UdpStream::WaitUntilReady() {
    fd_set fds;
    FD_SET(socket_, &fds);

    int ret = ::select(socket_ + 1, nullptr, &fds, nullptr, nullptr);
    if (!FD_ISSET(socket_, &fds))
        return false;

    return true;
}

Stream::Error UdpStream::Write(const uint8_t *data, unsigned int size) {
    auto bytes_sent = ::send(socket_, data, size, 0);

    // If we get an error back which relates to a possible congested
    // socket we try to resend one time and then fall into our actual
    // error handling.
    if (bytes_sent < 0) {
        switch (errno) {
        case ECONNREFUSED:
        case ENOPROTOOPT:
        case EPROTO:
        case EHOSTUNREACH:
        case ENETUNREACH:
        case ENETDOWN:
            MCS_DEBUG("Trying to resend due to a possible congested socket");
            bytes_sent = ::send(socket_, data, size, 0);
            break;
        default:
            break;
        }
    }

    if (bytes_sent < 0) {
        MCS_ERROR("Failed to send packet to remote: %s (%d)", std::strerror(-errno), errno);
        return Error::kFailed;
    }
    else if (bytes_sent == 0) {
        MCS_ERROR("Remote has closed connection: %s (%d)", std::strerror(-errno), errno);
        return Error::kRemoteClosedConnection;
    }

    return Error::kNone;
}

Port UdpStream::LocalPort() const {
    return local_port_;
}

} // namespace network
} // namespace mcs
