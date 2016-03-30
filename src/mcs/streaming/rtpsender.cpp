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

#include "mcs/streaming/rtpsender.h"

namespace {
static constexpr const char *kRTPSenderThreadName{"RTPSender"};
static constexpr unsigned int kRTPHeaderSize{12};
static constexpr unsigned int kMPEGTSPacketSize{188};
static constexpr unsigned int kSourceID = 0xdeadbeef;
// See http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml
static constexpr unsigned int kRTPPayloadTypeMP2T = 33;
}

namespace mcs {
namespace streaming {

RTPSender::RTPSender(const network::Stream::Ptr &stream, const video::SenderReport::Ptr &report) :
    stream_(stream),
    max_ts_packets_((stream->MaxUnitSize() - kRTPHeaderSize) / kMPEGTSPacketSize),
    report_(report),
    rtp_sequence_number_(0),
    queue_(video::BufferQueue::Create()){
}

RTPSender::~RTPSender() {
}

bool RTPSender::Start() {
    return true;
}

bool RTPSender::Stop() {
    return true;
}

bool RTPSender::Execute() {
    if (!queue_->WaitToBeFilled())
        return true;

    if (!stream_->WaitUntilReady())
        return true;

    queue_->Lock();

    while(true) {
        const auto packet = queue_->PopUnlocked();
        if (!packet)
            break;

        if (stream_->Write(packet->Data(), packet->Length()) != network::Stream::Error::kNone) {
            MCS_ERROR("Failed to send packet to remote");
            // FIXME possible the remote side disconected. Check and
            // bring everything down if that is the case.
            return false;
        }

        report_->SentPacket(packet->Timestamp(), packet->Length());
    }

    queue_->Unlock();

    return true;
}

bool RTPSender::Queue(const video::Buffer::Ptr &packets) {
    if (packets->Length() % kMPEGTSPacketSize != 0) {
        MCS_WARNING("Packet buffer has an invalid length %d", packets->Length());
        return false;
    }

    queue_->Lock();

    uint32_t offset = 0;
    while (offset < packets->Length()) {
        const auto packet = mcs::video::Buffer::Create(kRTPHeaderSize + max_ts_packets_ * kMPEGTSPacketSize);

        if (!packet->Data())
            continue;

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

        size_t num_ts_packets = (packets->Length() - offset) / kMPEGTSPacketSize;
        if (num_ts_packets > max_ts_packets_)
            num_ts_packets = max_ts_packets_;

        ::memcpy(&ptr[12], packets->Data() + offset, num_ts_packets * kMPEGTSPacketSize);

        packet->SetRange(0, kRTPHeaderSize + num_ts_packets * kMPEGTSPacketSize);

        // We're only setting the timestamp on the packet here for
        // statistically reasons we can check later on how late we
        // send a buffer out.
        packet->SetTimestamp(packets->Timestamp());

        offset += num_ts_packets * kMPEGTSPacketSize;

        queue_->PushUnlocked(packet);
    }

    queue_->Unlock();

    return true;
}

int32_t RTPSender::LocalPort() const {
    return stream_->LocalPort();
}

std::string RTPSender::Name() const {
    return kRTPSenderThreadName;
}

} // namespace streaming
} // namespace mcs
