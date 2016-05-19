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

#ifndef AC_NETWORK_STREAM_H_
#define AC_NETWORK_STREAM_H_

#include <memory>

#include "ac/non_copyable.h"
#include "ac/utils.h"

#include "ac/network/types.h"

namespace ac {
namespace network {

class Stream : public ac::NonCopyable {
public:
    typedef std::shared_ptr<Stream> Ptr;

    enum class Error {
        kNone,
        kFailed,
        kRemoteClosedConnection,
    };

    virtual bool Connect(const std::string &address, const Port &port) = 0;

    virtual bool WaitUntilReady() = 0;

    virtual Error Write(const uint8_t *data, unsigned int size,
                        const ac::TimestampUs &timestamp = 0) = 0;

    virtual Port LocalPort() const = 0;

    /**
     * @brief Returns the maximum size of a unit the stream will send out
     * @return Maximum send unit size in bytes
     */
    virtual std::uint32_t MaxUnitSize() const = 0;

protected:
    Stream() = default;
};

} // namespace network
} // namespace ac

#endif
