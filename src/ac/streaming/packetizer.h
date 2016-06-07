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

#ifndef AC_STREAMING_PACKETIZER_H_
#define AC_STREAMING_PACKETIZER_H_

#include "ac/non_copyable.h"

#include "ac/video/buffer.h"

namespace ac {
namespace streaming {

class Packetizer : public ac::NonCopyable {
public:
    typedef std::shared_ptr<Packetizer> Ptr;
    typedef int TrackId;

    class TrackFormat {
    public:
        TrackFormat(const std::string &mime = "",
                    unsigned int profile_idc = 0,
                    unsigned int level_idc = 0,
                    unsigned int constraint_set = 0) :
            mime(mime),
            profile_idc(profile_idc),
            level_idc(level_idc),
            constraint_set(constraint_set) {
        }

        TrackFormat(const TrackFormat &other) :
            mime(other.mime),
            profile_idc(other.profile_idc),
            level_idc(other.level_idc),
            constraint_set(other.constraint_set) {
        }

        bool operator==(const TrackFormat &rhs) const {
            return mime == rhs.mime &&
                    profile_idc == rhs.profile_idc &&
                    level_idc == rhs.level_idc &&
                    constraint_set == rhs.constraint_set;
        }

        std::string mime;
        unsigned int profile_idc;
        unsigned int level_idc;
        unsigned int constraint_set;
    };

    enum Flags {
        kEmitPATandPMT = 1,
        kEmitPCR = 2,
        kIsEncrypted = 4,
        kPrependSPSandPPStoIDRFrames = 8,
    };

    virtual TrackId AddTrack(const TrackFormat &format) = 0;
    virtual void SubmitCSD(TrackId track_index, const video::Buffer::Ptr &buffer) = 0;
    virtual bool Packetize(TrackId track_index, const video::Buffer::Ptr &access_unit,
                           video::Buffer::Ptr *packets, int flags = 0) = 0;
};

} // namespace streaming
} // namespace ac

#endif
