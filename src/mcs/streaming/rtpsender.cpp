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

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/tcp.h>
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

#include "mcs/video/statistics.h"

#include "mcs/streaming/rtpsender.h"

namespace {
static constexpr const char *kRTPSenderThreadName{"RTPSender"};
static constexpr unsigned int kMaxUDPPacketSize = 1472;
static constexpr unsigned int kMaxNumTSPacketsPerRTPPacket = (kMaxUDPPacketSize - 12) / 188;
static constexpr unsigned int kUdpTxBufferSize = 256 * 1024;
static constexpr unsigned int kSourceID = 0xdeadbeef;
// See http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml
static constexpr unsigned int kRTPPayloadTypeMP2T = 33;

static int32_t PickRandomRTPPort() {
    // Pick an even integer in range [1024, 65534)
    static const size_t range = (65534 - 1024) / 2;
    return (int32_t)(((float)(range + 1) * rand()) / RAND_MAX) * 2 + 1024;
}

int MakeSocketNonBlocking(int socket) {
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
namespace streaming {

TransportSender::Ptr RTPSender::Create(const std::string &address, int port) {
    return std::shared_ptr<RTPSender>(new RTPSender)->FinalizerConstruction(address, port);
}

RTPSender::RTPSender() :
    fd_(-1),
    running_(false),
    rtp_sequence_number_(0),
    queue_(video::BufferQueue::Create()),
    bytes_sent_(0),
    local_port_(0) {
}

RTPSender::~RTPSender() {
    running_ = false;

    worker_thread_.join();

    if (fd_ > 0)
        ::close(fd_);
}

RTPSender::Ptr RTPSender::FinalizerConstruction(const std::string &address, int port) {
    auto sp = shared_from_this();

    if (running_)
        return sp;

    MCS_DEBUG("Connected with remote on %s:%d", address, port);

    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0) {
        MCS_ERROR("Failed to create socket: %s (%d)",
                  ::strerror(-errno), errno);
        return nullptr;
    }

    int value = kUdpTxBufferSize;
    if (setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &value, sizeof(value)) < 0) {
        MCS_ERROR("Failed to adjust socket send buffer size");
        return nullptr;
    }

    if (MakeSocketNonBlocking(fd_) < 0) {
        MCS_ERROR("Failed to make socket non blocking: %s (%d)",
                  ::strerror(-errno), errno);
        return nullptr;
    }

    local_port_ = PickRandomRTPPort();

    struct sockaddr_in addr;
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(local_port_);

    if (::bind(fd_, (const struct sockaddr *) &addr, sizeof(addr)) < 0) {
        MCS_ERROR("Failed to bind socket to address 0.0.0.0:%d: %s (%d)",
                  local_port_, ::strerror(-errno), errno);
        return nullptr;
    }

    struct sockaddr_in remote_addr;
    memset(remote_addr.sin_zero, 0, sizeof(remote_addr.sin_zero));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);

    struct hostent *ent = gethostbyname(address.c_str());
    if (!ent) {
        MCS_ERROR("Failed to resolve remote address '%s': %s",
                  address, ::strerror(-errno), errno);
        return nullptr;
    }

    remote_addr.sin_addr.s_addr = *(in_addr_t*) ent->h_addr;

    if (::connect(fd_, (const struct sockaddr*) &remote_addr, sizeof(remote_addr)) < 0) {
        MCS_ERROR("Failed to connect to remote %s:%d: %s (%d)",
                  address, port, ::strerror(-errno), errno);
        return nullptr;
    }

    running_ = true;
    worker_thread_ = std::thread(&RTPSender::ThreadLoop, this);

    return sp;
}

void RTPSender::ThreadLoop() {
    mcs::Utils::SetThreadName(kRTPSenderThreadName);

    while (running_) {
        if (!queue_->WaitToBeFilled())
            continue;

        fd_set fds;
        FD_SET(fd_, &fds);

        int ret = select(fd_ + 1, nullptr, &fds, nullptr, nullptr);
        if (!FD_ISSET(fd_, &fds))
            continue;
        queue_->Lock();

        while(true) {
            auto packet = queue_->PopUnlocked();
            if (!packet)
                break;

            if (packet->Timestamp() > 0) {
                int64_t now = mcs::Utils::GetNowUs();
                int64_t diff = (now - packet->Timestamp()) / 1000ll;
                video::Statistics::Instance()->RecordRTPBufferSent(diff);
            }

            auto n = ::send(fd_, packet->Data(), packet->Length(), 0);

            // If we get an error back which relates to a possible congested
            // socket we try to resend one time and then fall into our actual
            // error handling.
            if (n < 0) {
                switch (errno) {
                case ECONNREFUSED:
                case ENOPROTOOPT:
                case EPROTO:
                case EHOSTUNREACH:
                case ENETUNREACH:
                case ENETDOWN:
                    MCS_DEBUG("Trying to resend due to a possible congested socket");
                    n = ::send(fd_, packet->Data(), packet->Length(), 0);
                    break;
                default:
                    break;
                }
            }

            if (n < 0) {
                MCS_ERROR("Failed to send packet to remote: %s (%d)",
                          ::strerror(-errno), errno);
                continue;
            }
            else if (n == 0) {
                MCS_ERROR("Remote has closed connection: %s (%d)",
                          ::strerror(-errno), errno);
                // FIXME wind this up the stack so that we close or
                // reinitiate the connection
                break;
            }

            bytes_sent_ += packet->Length();

            static mcs::TimestampUs last_count_time = mcs::Utils::GetNowNs();
            if (mcs::Utils::GetNowUs() - last_count_time > 1000000ll) {
                last_count_time = mcs::Utils::GetNowUs();
                int64_t in_mbit = (bytes_sent_ * 8) / 1000000ll;
                video::Statistics::Instance()->RecordRTPBandwidth(in_mbit);
                bytes_sent_ = 0;
            }
        }

        queue_->Unlock();
    }
}

bool RTPSender::Queue(const video::Buffer::Ptr &packets) {
    if (packets->Length() % 188 != 0) {
        MCS_WARNING("Packet buffer has an invalid length %d", packets->Length());
        return false;
    }

    queue_->Lock();

    uint32_t offset = 0;
    while (offset < packets->Length()) {
        auto packet = mcs::video::Buffer::Create(12 + kMaxNumTSPacketsPerRTPPacket * 188);

        uint8_t *ptr = packet->Data();

        ptr[0] = 0x80;
        ptr[1] = kRTPPayloadTypeMP2T;

        ptr[2] = (rtp_sequence_number_ >> 8) & 0xff;
        ptr[3] = rtp_sequence_number_ & 0xff;

        rtp_sequence_number_ = (rtp_sequence_number_ + 1) & 0xffff;

        // Adjust time to 90kHz
        uint32_t rtp_time = (mcs::Utils::GetNowUs() * 9) / 100ll;

        ptr[4] = rtp_time >> 24;
        ptr[5] = (rtp_time >> 16) & 0xff;
        ptr[6] = (rtp_time >> 8) & 0xff;
        ptr[7] = rtp_time & 0xff;

        ptr[8] = kSourceID >> 24;
        ptr[9] = (kSourceID >> 16) & 0xff;
        ptr[10] = (kSourceID >> 8) & 0xff;
        ptr[11] = kSourceID & 0xff;

        size_t num_ts_packets = (packets->Length() - offset) / 188;
        if (num_ts_packets > kMaxNumTSPacketsPerRTPPacket) {
            num_ts_packets = kMaxNumTSPacketsPerRTPPacket;
        }

        ::memcpy(&ptr[12], packets->Data() + offset, num_ts_packets * 188);

        packet->SetRange(0, 12 + num_ts_packets * 188);

        // We're only setting the timestamp on the packet here for
        // statistically reasons we can check later on how late we
        // send a buffer out.
        packet->SetTimestamp(packets->Timestamp());

        offset += num_ts_packets * 188;

        queue_->PushUnlocked(packet);
    }

    queue_->Unlock();

    if (packets->Timestamp() == 0)
        return true;

    int64_t now = mcs::Utils::GetNowUs();
    int64_t diff = (now - packets->Timestamp()) / 1000ll;
    video::Statistics::Instance()->RecordRTPBufferQueued(diff);

    return true;
}

int32_t RTPSender::LocalPort() const {
    return local_port_;
}

} // namespace streaming
} // namespace mcs
