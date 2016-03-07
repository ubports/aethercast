/*
 * Copyright 2012, The Android Open Source Project
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * NOTE: The implementation is based on the Android implementation for WiFi
 * display support in frameworks/av/libstagefright/wifi-display/TSPacketizer.cpp
 * and is adjusted for our needs.
 */

#include <arpa/inet.h>
#include <memory.h>

#include "mcs/utils.h"
#include "mcs/logger.h"

#include "mcs/video/utils.h"

#include "mcs/streaming/mpegtspacketizer.h"

namespace {
static constexpr uint8_t kH264NALPrefix[] = { 0x00, 0x00, 0x00, 0x01 };

// One byte for each profile, level and constraint
static constexpr uint32_t kCSDSize{3};

// See WiFi Display spec version 1.1 chapter D.4.2 PAT/PMT
static constexpr unsigned int kPIDofPMT{0x100};
static constexpr unsigned int kPIDofPCR{0x1000};
static constexpr unsigned int kVideoPIDStart{0x1011};

static constexpr unsigned int kH264StreamType{0x1b};
static constexpr unsigned int kVideoStreamIdStart{0xe0};
static constexpr unsigned int kVideoStreamIdStop{0xef};
static constexpr unsigned int kAVCVideoDescriptorTag{40};
static constexpr unsigned int kAVCTimingAndHRDDescriptor{42};
}

namespace mcs {
namespace streaming {

struct MPEGTSPacketizer::Track {
    typedef std::shared_ptr<Track> Ptr;

    static Ptr Create(const TrackFormat &format, unsigned int pid,
                      unsigned int stream_type, unsigned int stream_id);

    unsigned int NextContinuityCounter();

    bool IsAudio() const { return mcs::Utils::StringStartsWith(format.mime, "audio/"); }
    bool IsVideo() const { return mcs::Utils::StringStartsWith(format.mime, "video/"); }

    bool IsH264() const { return format.mime == "video/avc"; }

    void SubmitCSD(const mcs::video::Buffer::Ptr &buffer);

    mcs::video::Buffer::Ptr PrependCSD(const mcs::video::Buffer::Ptr &buffer) const;

    void Finalize();

    Track(const TrackFormat &format, unsigned int pid,
          unsigned int stream_type, unsigned int stream_id);

    TrackFormat format;
    unsigned int pid;
    unsigned int stream_type;
    unsigned int stream_id;
    unsigned int continuity_counter;
    bool finalized;
    std::vector<mcs::video::Buffer::Ptr> csd;
    std::vector<mcs::video::Buffer::Ptr> descriptors;
};

MPEGTSPacketizer::Track::Ptr MPEGTSPacketizer::Track::Create(const TrackFormat &format, unsigned int pid,
                                                             unsigned int stream_type, unsigned int stream_id) {
    return std::shared_ptr<Track>(new Track(format, pid, stream_type, stream_id));
}

MPEGTSPacketizer::Track::Track(const TrackFormat &format, unsigned int pid,
                               unsigned int stream_type, unsigned int stream_id) :
    format(format),
    pid(pid),
    stream_type(stream_type),
    stream_id(stream_id),
    continuity_counter(0),
    finalized(false) {
}

unsigned int MPEGTSPacketizer::Track::NextContinuityCounter() {
    unsigned int prev = continuity_counter;
    if (++continuity_counter == 16)
        continuity_counter = 0;
    return prev;
}

void MPEGTSPacketizer::Track::SubmitCSD(const video::Buffer::Ptr &buffer) {
    if (!IsH264())
        return;

    const uint8_t *data = buffer->Data();
    size_t size = buffer->Length();

    const uint8_t *nal_start;
    size_t nal_size;

    while (mcs::video::GetNextNALUnit(&data, &size, &nal_start, &nal_size, true)) {
        auto current = mcs::video::Buffer::Create(nal_size + sizeof(kH264NALPrefix));

        ::memcpy(current->Data(), kH264NALPrefix, sizeof(kH264NALPrefix));
        ::memcpy(current->Data() + sizeof(kH264NALPrefix), nal_start, nal_size);

        csd.push_back(current);
    }
}

mcs::video::Buffer::Ptr MPEGTSPacketizer::Track::PrependCSD(const mcs::video::Buffer::Ptr &buffer) const {
    size_t size = 0;
    for (auto current : csd)
        size += current->Length();

    auto new_buffer = mcs::video::Buffer::Create(buffer->Length() + size);
    size_t offset = 0;
    for (auto current : csd) {
        ::memcpy(new_buffer->Data() + offset, current->Data(), current->Length());
        offset += current->Length();
    }

    ::memcpy(new_buffer->Data() + offset, buffer->Data(), buffer->Length());

    return new_buffer;
}

void MPEGTSPacketizer::Track::Finalize() {
    if (finalized)
        return;

    if(!IsH264())
        return;

    MCS_DEBUG("");

    {
        // AVC video descriptor (40)
        const auto descriptor = mcs::video::Buffer::Create(6);
        uint8_t *data = descriptor->Data();
        data[0] = kAVCVideoDescriptorTag;  // descriptor_tag
        data[1] = 4;  // descriptor_length

        if (csd.size() > 0) {
            // Seems to be a conventation that the first NAL we get
            // submitted as part of the codec-specific data is the
            // SPS we want here.
            auto sps = csd.at(0);
            // We skip the first four bytes (NAL preamble) and then
            // just copy profile/constraint/level settings (one byte
            // each).
            memcpy(&data[2], sps->Data() + sizeof(kH264NALPrefix), kCSDSize);
        }
        else {
            data[2] = format.profile_idc;    // profile_idc
            data[3] = format.constraint_set; // constraint_set*
            data[4] = format.level_idc;      // level_idc
        }

        // AVC_still_present=0, AVC_24_hour_picture_flag=0, reserved
        data[5] = 0x3f;

        descriptors.push_back(descriptor);
    }
    {
        // AVC timing and HRD descriptor (42)

        auto descriptor = mcs::video::Buffer::Create(4);
        uint8_t *data = descriptor->Data();
        data[0] = kAVCTimingAndHRDDescriptor;  // descriptor_tag
        data[1] = 2;  // descriptor_length

        // hrd_management_valid_flag = 0
        // reserved = 111111b
        // picture_and_timing_info_present = 0
        data[2] = 0x7e;

        // fixed_frame_rate_flag = 0
        // temporal_poc_flag = 0
        // picture_to_display_conversion_flag = 0
        // reserved = 11111b
        data[3] = 0x1f;

        descriptors.push_back(descriptor);
    }

    finalized = true;
}

Packetizer::Ptr MPEGTSPacketizer::Create() {
    return std::shared_ptr<Packetizer>(new MPEGTSPacketizer);
}

MPEGTSPacketizer::MPEGTSPacketizer() :
    pat_continuity_counter_(0),
    pmt_continuity_counter_(0) {
    InitCrcTable();
}

MPEGTSPacketizer::~MPEGTSPacketizer() {
}

MPEGTSPacketizer::TrackId MPEGTSPacketizer::AddTrack(const TrackFormat &format) {
    auto is_video = mcs::Utils::StringStartsWith(format.mime, "video/");

    if (!is_video) {
        MCS_ERROR("Audio tracks for MPEGTS are currently not supported");
        return TrackId(-1);
    }

    if (format.mime != "video/avc") {
        MCS_ERROR("Video formats other than video/avc are not supported");
        return TrackId(-1);
    }

    // First PID as per WiFi Display spec
    unsigned int pid_start = kVideoPIDStart;
    unsigned int stream_type = kH264StreamType;
    unsigned int stream_id_start = kVideoStreamIdStart;
    unsigned int stream_id_stop = kVideoStreamIdStop;

    unsigned int num_same_tracks = 0;
    unsigned int pid = pid_start;

    for (auto track : tracks_) {
        if (track->stream_type == stream_type)
            num_same_tracks++;

        if (track->IsAudio() || track->IsVideo())
            pid++;
    }

    unsigned int stream_id = stream_id_start + num_same_tracks;
    if (stream_id > stream_id_stop) {
        MCS_ERROR("All stream ids are in use");
        return TrackId(-1);
    }

    auto track = Track::Create(format, pid, stream_type, stream_id);
    tracks_.push_back(track);

    return tracks_.size() - 1;
}

void MPEGTSPacketizer::SubmitCSD(TrackId track_index, const video::Buffer::Ptr &buffer) {
    if (track_index > tracks_.size() -1)
        return;

    auto track = tracks_.at(track_index);
    track->SubmitCSD(buffer);
}

bool MPEGTSPacketizer::Packetize(TrackId track_index, const video::Buffer::Ptr &_access_unit,
                                 video::Buffer::Ptr *packets, int flags) {
    size_t numStuffingBytes = 0;
    const uint8_t *PES_private_data = nullptr;
    size_t PES_private_data_len = 0;
    mcs::video::Buffer::Ptr access_unit = _access_unit;
    int64_t timeUs = access_unit->Timestamp();

    packets->reset();

    if (track_index > tracks_.size() - 1) {
        MCS_ERROR("Invalid track index %d supplied", track_index);
        return false;
    }

    auto track = tracks_.at(track_index);

    if (track->IsH264() && (flags & Flags::kPrependSPSandPPStoIDRFrames)
            && mcs::video::DoesBufferContainIDRFrame(access_unit)) {
        // prepend codec specific data, i.e. SPS and PPS.
        access_unit = track->PrependCSD(access_unit);
    }

    // 0x47
    // transport_error_indicator = b0
    // payload_unit_start_indicator = b1
    // transport_priority = b0
    // PID
    // transport_scrambling_control = b00
    // adaptation_field_control = b??
    // continuity_counter = b????
    // -- payload follows
    // packet_startcode_prefix = 0x000001
    // stream_id
    // PES_packet_length = 0x????
    // reserved = b10
    // PES_scrambling_control = b00
    // PES_priority = b0
    // data_alignment_indicator = b1
    // copyright = b0
    // original_or_copy = b0
    // PTS_DTS_flags = b10  (PTS only)
    // ESCR_flag = b0
    // ES_rate_flag = b0
    // DSM_trick_mode_flag = b0
    // additional_copy_info_flag = b0
    // PES_CRC_flag = b0
    // PES_extension_flag = b0
    // PES_header_data_length = 0x05
    // reserved = b0010 (PTS)
    // PTS[32..30] = b???
    // reserved = b1
    // PTS[29..15] = b??? ???? ???? ???? (15 bits)
    // reserved = b1
    // PTS[14..0] = b??? ???? ???? ???? (15 bits)
    // reserved = b1
    // the first fragment of "buffer" follows

    // Each transport packet (except for the last one contributing to the PES
    // payload) must contain a multiple of 16 bytes of payload per HDCP spec.
    bool alignPayload = false;

    /*
       a) The very first PES transport stream packet contains

       4 bytes of TS header
       ... padding
       14 bytes of static PES header
       PES_private_data_len + 1 bytes (only if PES_private_data_len > 0)
       numStuffingBytes bytes

       followed by the payload

       b) Subsequent PES transport stream packets contain

       4 bytes of TS header
       ... padding

       followed by the payload
    */

    size_t PES_packet_length = access_unit->Length() + 8 + numStuffingBytes;
    if (PES_private_data_len > 0)
        PES_packet_length += PES_private_data_len + 1;

    size_t numTSPackets = 1;

    {
        // Make sure the PES header fits into a single TS packet:
        size_t PES_header_size = 14 + numStuffingBytes;
        if (PES_private_data_len > 0) {
            PES_header_size += PES_private_data_len + 1;
        }

        if (PES_header_size > 188u - 4u)
            MCS_FATAL("Invalid header size");

        size_t sizeAvailableForPayload = 188 - 4 - PES_header_size;
        size_t numBytesOfPayload = access_unit->Length();

        if (numBytesOfPayload > sizeAvailableForPayload) {
            numBytesOfPayload = sizeAvailableForPayload;

            if (alignPayload && numBytesOfPayload > 16) {
                numBytesOfPayload -= (numBytesOfPayload % 16);
            }
        }

        size_t numBytesOfPayloadRemaining = access_unit->Length() - numBytesOfPayload;

        // This is how many bytes of payload each subsequent TS packet
        // can contain at most.
        sizeAvailableForPayload = 188 - 4;
        size_t sizeAvailableForAlignedPayload = sizeAvailableForPayload;
        if (alignPayload) {
            // We're only going to use a subset of the available space
            // since we need to make each fragment a multiple of 16 in size.
            sizeAvailableForAlignedPayload -=
                (sizeAvailableForAlignedPayload % 16);
        }

        size_t numFullTSPackets =
            numBytesOfPayloadRemaining / sizeAvailableForAlignedPayload;

        numTSPackets += numFullTSPackets;

        numBytesOfPayloadRemaining -=
            numFullTSPackets * sizeAvailableForAlignedPayload;

        // numBytesOfPayloadRemaining < sizeAvailableForAlignedPayload
        if (numFullTSPackets == 0 && numBytesOfPayloadRemaining > 0) {
            // There wasn't enough payload left to form a full aligned payload,
            // the last packet doesn't have to be aligned.
            ++numTSPackets;
        } else if (numFullTSPackets > 0
                && numBytesOfPayloadRemaining
                    + sizeAvailableForAlignedPayload > sizeAvailableForPayload) {
            // The last packet emitted had a full aligned payload and together
            // with the bytes remaining does exceed the unaligned payload
            // size, so we need another packet.
            ++numTSPackets;
        }
    }

    if (flags & Flags::kEmitPATandPMT)
        numTSPackets += 2;

    if (flags & Flags::kEmitPCR)
        ++numTSPackets;

    auto buffer = mcs::video::Buffer::Create(numTSPackets * 188);
    buffer->SetTimestamp(access_unit->Timestamp());

    uint8_t *packetDataStart = buffer->Data();

    if (flags & Flags::kEmitPATandPMT) {
        // Program Association Table (PAT):
        // 0x47
        // transport_error_indicator = b0
        // payload_unit_start_indicator = b1
        // transport_priority = b0
        // PID = b0000000000000 (13 bits)
        // transport_scrambling_control = b00
        // adaptation_field_control = b01 (no adaptation field, payload only)
        // continuity_counter = b????
        // skip = 0x00
        // --- payload follows
        // table_id = 0x00
        // section_syntax_indicator = b1
        // must_be_zero = b0
        // reserved = b11
        // section_length = 0x00d
        // transport_stream_id = 0x0000
        // reserved = b11
        // version_number = b00001
        // current_next_indicator = b1
        // section_number = 0x00
        // last_section_number = 0x00
        //   one program follows:
        //   program_number = 0x0001
        //   reserved = b111
        //   program_map_PID = kPID_PMT (13 bits!)
        // CRC = 0x????????

        if (++pat_continuity_counter_ == 16)
            pat_continuity_counter_ = 0;

        uint8_t *ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x40;
        *ptr++ = 0x00;
        *ptr++ = 0x10 | pat_continuity_counter_;
        *ptr++ = 0x00;

        uint8_t *crcDataStart = ptr;
        *ptr++ = 0x00;
        *ptr++ = 0xb0;
        *ptr++ = 0x0d;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0xc3;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0x01;
        *ptr++ = 0xe0 | (kPIDofPMT >> 8);
        *ptr++ = kPIDofPMT & 0xff;

        if (ptr - crcDataStart != 12)
            MCS_FATAL("Invalid position for ptr");

        uint32_t crc = ::htonl(CalcCrc32(crcDataStart, ptr - crcDataStart));
        ::memcpy(ptr, &crc, 4);
        ptr += 4;

        size_t sizeLeft = packetDataStart + 188 - ptr;
        ::memset(ptr, 0xff, sizeLeft);

        packetDataStart += 188;

        // Program Map (PMT):
        // 0x47
        // transport_error_indicator = b0
        // payload_unit_start_indicator = b1
        // transport_priority = b0
        // PID = kPID_PMT (13 bits)
        // transport_scrambling_control = b00
        // adaptation_field_control = b01 (no adaptation field, payload only)
        // continuity_counter = b????
        // skip = 0x00
        // -- payload follows
        // table_id = 0x02
        // section_syntax_indicator = b1
        // must_be_zero = b0
        // reserved = b11
        // section_length = 0x???
        // program_number = 0x0001
        // reserved = b11
        // version_number = b00001
        // current_next_indicator = b1
        // section_number = 0x00
        // last_section_number = 0x00
        // reserved = b111
        // PCR_PID = kPCR_PID (13 bits)
        // reserved = b1111
        // program_info_length = 0x???
        //   program_info_descriptors follow
        // one or more elementary stream descriptions follow:
        //   stream_type = 0x??
        //   reserved = b111
        //   elementary_PID = b? ???? ???? ???? (13 bits)
        //   reserved = b1111
        //   ES_info_length = 0x000
        // CRC = 0x????????

        if (++pmt_continuity_counter_ == 16)
            pmt_continuity_counter_ = 0;

        ptr = packetDataStart;

        *ptr++ = 0x47;
        *ptr++ = 0x40 | (kPIDofPMT >> 8);
        *ptr++ = kPIDofPMT & 0xff;
        *ptr++ = 0x10 | pmt_continuity_counter_;
        *ptr++ = 0x00;

        crcDataStart = ptr;
        *ptr++ = 0x02;

        *ptr++ = 0x00;  // section_length to be filled in below.
        *ptr++ = 0x00;

        *ptr++ = 0x00;
        *ptr++ = 0x01;
        *ptr++ = 0xc3;
        *ptr++ = 0x00;
        *ptr++ = 0x00;
        *ptr++ = 0xe0 | (kPIDofPCR >> 8);
        *ptr++ = kPIDofPCR & 0xff;

        size_t program_info_length = 0;
        for (auto descriptor : program_info_descriptors_)
            program_info_length += descriptor->Length();

        if(program_info_length >= 0x400)
            MCS_FATAL("Invalid length for program info");

        *ptr++ = 0xf0 | (program_info_length >> 8);
        *ptr++ = (program_info_length & 0xff);

        for (auto descriptor : program_info_descriptors_) {
            ::memcpy(ptr, descriptor->Data(), descriptor->Length());
            ptr += descriptor->Length();
        }

        for (auto track : tracks_) {
            // Make sure all the decriptors have been added.
            track->Finalize();

            *ptr++ = track->stream_type;
            *ptr++ = 0xe0 | (track->pid >> 8);
            *ptr++ = track->pid & 0xff;

            size_t ES_info_length = 0;
            for (auto descriptor : track->descriptors)
                ES_info_length += descriptor->Length();

            if (ES_info_length > 0xfff)
                MCS_FATAL("Invalid ES length %d", ES_info_length);

            *ptr++ = 0xf0 | (ES_info_length >> 8);
            *ptr++ = (ES_info_length & 0xff);

            for (auto descriptor : track->descriptors) {
                memcpy(ptr, descriptor->Data(), descriptor->Length());
                ptr += descriptor->Length();
            }
        }

        size_t section_length = ptr - (crcDataStart + 3) + 4 /* CRC */;

        crcDataStart[1] = 0xb0 | (section_length >> 8);
        crcDataStart[2] = section_length & 0xff;

        crc = ::htonl(CalcCrc32(crcDataStart, ptr - crcDataStart));
        memcpy(ptr, &crc, 4);
        ptr += 4;

        sizeLeft = packetDataStart + 188 - ptr;
        memset(ptr, 0xff, sizeLeft);

        packetDataStart += 188;
    }

    if (flags & Flags::kEmitPCR) {
        // PCR stream
        // 0x47
        // transport_error_indicator = b0
        // payload_unit_start_indicator = b1
        // transport_priority = b0
        // PID = kPCR_PID (13 bits)
        // transport_scrambling_control = b00
        // adaptation_field_control = b10 (adaptation field only, no payload)
        // continuity_counter = b0000 (does not increment)
        // adaptation_field_length = 183
        // discontinuity_indicator = b0
        // random_access_indicator = b0
        // elementary_stream_priority_indicator = b0
        // PCR_flag = b1
        // OPCR_flag = b0
        // splicing_point_flag = b0
        // transport_private_data_flag = b0
        // adaptation_field_extension_flag = b0
        // program_clock_reference_base = b?????????????????????????????????
        // reserved = b111111
        // program_clock_reference_extension = b?????????

        int64_t nowUs = mcs::Utils::GetNowUs();
        uint64_t PCR = nowUs * 27;  // PCR based on a 27MHz clock
        uint64_t PCR_base = PCR / 300;
        uint32_t PCR_ext = PCR % 300;

        uint8_t *ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x40 | (kPIDofPCR >> 8);
        *ptr++ = kPIDofPCR & 0xff;
        *ptr++ = 0x20;
        *ptr++ = 0xb7;  // adaptation_field_length
        *ptr++ = 0x10;
        *ptr++ = (PCR_base >> 25) & 0xff;
        *ptr++ = (PCR_base >> 17) & 0xff;
        *ptr++ = (PCR_base >> 9) & 0xff;
        *ptr++ = ((PCR_base & 1) << 7) | 0x7e | ((PCR_ext >> 8) & 1);
        *ptr++ = (PCR_ext & 0xff);

        size_t sizeLeft = packetDataStart + 188 - ptr;
        ::memset(ptr, 0xff, sizeLeft);

        packetDataStart += 188;
    }

    // Adjust time to 90kHz
    uint64_t PTS = (timeUs * 9ll) / 100ll;

    if (PES_packet_length >= 65536) {
        // This really should only happen for video.
        if (!track->IsVideo())
            MCS_FATAL("PES packet length too hight; should only happen for video (track %d mime %s)",
                      track_index, track->format.mime);

        MCS_WARNING("Reset PES packet length to 0");

        // It's valid to set this to 0 for video according to the specs.
        PES_packet_length = 0;
    }

    size_t sizeAvailableForPayload = 188 - 4 - 14 - numStuffingBytes;
    if (PES_private_data_len > 0) {
        sizeAvailableForPayload -= PES_private_data_len + 1;
    }

    size_t copy = access_unit->Length();

    if (copy > sizeAvailableForPayload) {
        copy = sizeAvailableForPayload;

        if (alignPayload && copy > 16) {
            copy -= (copy % 16);
        }
    }

    size_t numPaddingBytes = sizeAvailableForPayload - copy;

    uint8_t *ptr = packetDataStart;
    *ptr++ = 0x47;
    *ptr++ = 0x40 | (track->pid >> 8);
    *ptr++ = track->pid & 0xff;

    *ptr++ = (numPaddingBytes > 0 ? 0x30 : 0x10)
                | track->NextContinuityCounter();

    if (numPaddingBytes > 0) {
        *ptr++ = numPaddingBytes - 1;
        if (numPaddingBytes >= 2) {
            *ptr++ = 0x00;
            ::memset(ptr, 0xff, numPaddingBytes - 2);
            ptr += numPaddingBytes - 2;
        }
    }

    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0x01;
    *ptr++ = track->stream_id;
    *ptr++ = PES_packet_length >> 8;
    *ptr++ = PES_packet_length & 0xff;
    *ptr++ = 0x84;
    *ptr++ = (PES_private_data_len > 0) ? 0x81 : 0x80;

    size_t headerLength = 0x05 + numStuffingBytes;
    if (PES_private_data_len > 0) {
        headerLength += 1 + PES_private_data_len;
    }

    *ptr++ = headerLength;

    *ptr++ = 0x20 | (((PTS >> 30) & 7) << 1) | 1;
    *ptr++ = (PTS >> 22) & 0xff;
    *ptr++ = (((PTS >> 15) & 0x7f) << 1) | 1;
    *ptr++ = (PTS >> 7) & 0xff;
    *ptr++ = ((PTS & 0x7f) << 1) | 1;

    if (PES_private_data_len > 0) {
        *ptr++ = 0x8e;  // PES_private_data_flag, reserved.
        ::memcpy(ptr, PES_private_data, PES_private_data_len);
        ptr += PES_private_data_len;
    }

    for (size_t i = 0; i < numStuffingBytes; ++i) {
        *ptr++ = 0xff;
    }

    ::memcpy(ptr, access_unit->Data(), copy);
    ptr += copy;

    if (ptr != packetDataStart + 188)
        MCS_FATAL("Invalid pointer %p", ptr);

    packetDataStart += 188;

    size_t offset = copy;
    while (offset < access_unit->Length()) {
        // for subsequent fragments of "buffer":
        // 0x47
        // transport_error_indicator = b0
        // payload_unit_start_indicator = b0
        // transport_priority = b0
        // PID = b0 0001 1110 ???? (13 bits) [0x1e0 + 1 + sourceIndex]
        // transport_scrambling_control = b00
        // adaptation_field_control = b??
        // continuity_counter = b????
        // the fragment of "buffer" follows.

        size_t sizeAvailableForPayload = 188 - 4;

        size_t copy = access_unit->Length() - offset;

        if (copy > sizeAvailableForPayload) {
            copy = sizeAvailableForPayload;

            if (alignPayload && copy > 16) {
                copy -= (copy % 16);
            }
        }

        size_t numPaddingBytes = sizeAvailableForPayload - copy;

        uint8_t *ptr = packetDataStart;
        *ptr++ = 0x47;
        *ptr++ = 0x00 | (track->pid >> 8);
        *ptr++ = track->pid & 0xff;

        *ptr++ = (numPaddingBytes > 0 ? 0x30 : 0x10)
                    | track->NextContinuityCounter();

        if (numPaddingBytes > 0) {
            *ptr++ = numPaddingBytes - 1;
            if (numPaddingBytes >= 2) {
                *ptr++ = 0x00;
                memset(ptr, 0xff, numPaddingBytes - 2);
                ptr += numPaddingBytes - 2;
            }
        }

        memcpy(ptr, access_unit->Data() + offset, copy);
        ptr += copy;
        if (ptr != packetDataStart + 188)
            MCS_FATAL("Invalid pointer position %p", ptr);

        offset += copy;
        packetDataStart += 188;
    }

    if (packetDataStart != buffer->Data() + buffer->Length())
        MCS_FATAL("Invalid packet start position");

    *packets = buffer;

    report_->PacketizedFrame(buffer->Timestamp());

    return true;
}

void MPEGTSPacketizer::InitCrcTable() {
    uint32_t poly = 0x04C11DB7;

    for (int i = 0; i < 256; i++) {
        uint32_t crc = i << 24;
        for (int j = 0; j < 8; j++) {
            crc = (crc << 1) ^ ((crc & 0x80000000) ? (poly) : 0);
        }
        crc_table_[i] = crc;
    }
}

uint32_t MPEGTSPacketizer::CalcCrc32(const uint8_t *start, size_t size) const {
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *p;

    for (p = start; p < start + size; ++p) {
        crc = (crc << 8) ^ crc_table_[((crc >> 24) ^ *p) & 0xFF];
    }

    return crc;
}

} // namespace streaming
} // namespace mcs
