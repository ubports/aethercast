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

#ifndef MCS_STREAMING_MPEGTSPACKETIZER_H_
#define MCS_STREAMING_MPEGTSPACKETIZER_H_

#include <memory>
#include <vector>

#include "mcs/streaming/packetizer.h"

namespace mcs {
namespace streaming {

class MPEGTSPacketizer : public Packetizer {
public:
    static Packetizer::Ptr Create();

    ~MPEGTSPacketizer();

    TrackId AddTrack(const TrackFormat &format) override;

    void SubmitCSD(TrackId track_index, const video::Buffer::Ptr &buffer) override;

    bool Packetize(TrackId track_index, const video::Buffer::Ptr &access_unit,
                   video::Buffer::Ptr *packets, int flags = 0) override;

private:
    MPEGTSPacketizer();

private:
    void InitCrcTable();
    uint32_t CalcCrc32(const uint8_t *start, size_t size) const;

private:
    struct Track;

private:
    unsigned int pat_continuity_counter_;
    unsigned int pmt_continuity_counter_;
    uint32_t crc_table_[256];
    std::vector<std::shared_ptr<Track>> tracks_;
    std::vector<video::Buffer::Ptr> program_info_descriptors_;
};

} // namespace streaming
} // namespace mcs

#endif
