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

#ifndef MCS_STREAMING_TRANSPORTSENDER_H_
#define MCS_STREAMING_TRANSPORTSENDER_H_

#include "mcs/non_copyable.h"

#include "mcs/video/buffer.h"

namespace mcs {
namespace streaming {

class TransportSender : public mcs::NonCopyable {
public:
    typedef std::shared_ptr<TransportSender> Ptr;

    virtual bool Queue(const mcs::video::Buffer::Ptr &packets) = 0;
    virtual int32_t LocalPort() const = 0;
};

} // namespace streaming
} // namespace mcs

#endif
