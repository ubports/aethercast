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

#include <random>

#include <boost/concept_check.hpp>

#include "mcs/logger.h"
#include "mcs/networkutils.h"

#include "mcs/network/udpstream.h"

namespace {
static constexpr unsigned int kUdpTxBufferSize = 256 * 1024;
/* Value below configured MTU so that we don't require any further splits */
static constexpr unsigned int kMaxUDPPacketSize = 1472;
}

namespace mcs {
namespace network {

UdpStream::UdpStream() :
    socket_(0),
    local_port_(NetworkUtils::PickRandomPort()) {
}

UdpStream::~UdpStream() {
    if (socket_ > 0)
        ::close(socket_);
}

bool UdpStream::Connect(const std::string &address, const Port &port) {
    MCS_DEBUG("Connected with remote on %s:%d", address, port);

    socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ < 0) {
        MCS_ERROR("Failed to create socket: %s (%d)", ::strerror(errno), errno);
        return false;
    }

    int value = kUdpTxBufferSize;
    if (::setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &value, sizeof(value)) < 0) {
        MCS_ERROR("Failed to set socket transmit buffer size: %s (%d)", ::strerror(errno), errno);
        return false;
    }

    if (NetworkUtils::MakeSocketNonBlocking(socket_) < 0) {
        MCS_ERROR("Failed to make socket non blocking");
        return false;
    }

    struct sockaddr_in addr;
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(local_port_);

    if (::bind(socket_, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        MCS_ERROR("Failed to bind socket to address: %s (%d)", ::strerror(errno), errno);
        return false;
    }

    struct sockaddr_in remote_addr;
    memset(remote_addr.sin_zero, 0, sizeof(remote_addr.sin_zero));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);

    struct hostent *ent = gethostbyname(address.c_str());
    if (!ent) {
        MCS_ERROR("Failed to resolve remote address");
        return false;
    }

    remote_addr.sin_addr.s_addr = *(in_addr_t*) ent->h_addr;

    if (::connect(socket_, reinterpret_cast<const struct sockaddr*>(&remote_addr), sizeof(remote_addr)) < 0) {
        MCS_ERROR("Failed to connect to remote: %s (%d)", ::strerror(errno), errno);
        return false;
    }

    return true;
}

bool UdpStream::WaitUntilReady() {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socket_, &fds);

    const int ret = ::select(socket_ + 1, nullptr, &fds, nullptr, nullptr);
    if (ret < 0 || !FD_ISSET(socket_, &fds))
        return false;

    return true;
}

Stream::Error UdpStream::Write(const uint8_t *data, unsigned int size,
                               const mcs::TimestampUs &timestamp) {

    boost::ignore_unused_variable_warning(timestamp);

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
        MCS_ERROR("Failed to send packet to remote: %s (%d)", ::strerror(-errno), errno);
        return Error::kFailed;
    }
    else if (bytes_sent == 0) {
        MCS_ERROR("Remote has closed connection: %s (%d)", ::strerror(-errno), errno);
        return Error::kRemoteClosedConnection;
    }

    return Error::kNone;
}

Port UdpStream::LocalPort() const {
    return local_port_;
}

std::uint32_t UdpStream::MaxUnitSize() const {
    return kMaxUDPPacketSize;
}

} // namespace network
} // namespace mcs
