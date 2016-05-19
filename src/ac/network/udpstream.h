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

#ifndef AC_NETWORK_UDPSTREAM_H_
#define AC_NETWORK_UDPSTREAM_H_

#include <memory>

#include "ac/non_copyable.h"

#include "ac/network/stream.h"

namespace ac {
namespace network {

class UdpStream : public Stream {
public:
    UdpStream();
    ~UdpStream();

    bool Connect(const std::string &address, const Port &port) override;

    bool WaitUntilReady() override;

    Error Write(const uint8_t *data, unsigned int size,
                const ac::TimestampUs &timestamp = 0) override;

    Port LocalPort() const override;

    std::uint32_t MaxUnitSize() const override;

private:
    int socket_;
    Port local_port_;
};

} // namespace network
} // namespace ac

#endif
