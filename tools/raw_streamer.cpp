/*
 *  Copyright (C) 2016 Canonical, Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory.h>

#include <memory>
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

#include <mcs/logger.h>

#include <mcs/network/udpstream.h>

#include <mcs/report/reportfactory.h>

#include <mcs/streaming/mpegtspacketizer.h>
#include <mcs/streaming/rtpsender.h>

namespace {
static constexpr unsigned int kBufferSize = 2048;
}

int main(int argc, char **argv) {

    if (argc != 2 || argv[1] == nullptr) {
        std::cout << "Usage: " << std::endl
                  << " " << argv[0] << " <input>" << std::endl;
        return -EINVAL;
    }

    int fin = ::open(argv[1], O_RDONLY);
    if (fin < 0) {
        MCS_ERROR("Failed to open input file %s", argv[1]);
        return -EINVAL;
    }

    auto report_factory = mcs::report::ReportFactory::Create();

    auto packetizer = mcs::streaming::MPEGTSPacketizer::Create(report_factory->CreatePacketizerReport());
    auto udp_stream = std::make_shared<mcs::network::UdpStream>(mcs::IpV4Address::from_string("192.168.178.1"), 5000);
    auto sender = std::make_shared<mcs::streaming::RTPSender>(udp_stream, report_factory->CreateSenderReport());

    if (!sender)
        return -EIO;

    int track_index = packetizer->AddTrack(mcs::streaming::MPEGTSPacketizer::TrackFormat{"video/avc"});

    int counter = 0;

    while (true) {
        uint8_t inbuf[kBufferSize];

        auto bytes_read = ::read(fin, inbuf, kBufferSize);
        if (bytes_read < 0) {
            MCS_ERROR("Failed to read input data");
            break;
        }
        else if (bytes_read == 0)
            break;

        auto buffer = mcs::video::Buffer::Create(bytes_read, mcs::Utils::GetNowUs());
        ::memcpy(buffer->Data(), inbuf, bytes_read);

        mcs::video::Buffer::Ptr outbuf;

        int flags = 0;
        if (!(counter % 100))
            flags = mcs::streaming::MPEGTSPacketizer::kEmitPCR |
                    mcs::streaming::MPEGTSPacketizer::kEmitPATandPMT;

        packetizer->Packetize(track_index, buffer, &outbuf, flags);
        sender->Queue(outbuf);

        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    ::close(fin);

    return 0;
}
