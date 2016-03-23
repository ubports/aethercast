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

#ifndef W11TNG_DHCPLEASEPARSER_H_
#define W11TNG_DHCPLEASEPARSER_H_

#include <string>
#include <vector>

#include <mcs/glib_wrapper.h>

#include <mcs/ip_v4_address.h>

namespace w11tng {
class DhcpLeaseInfo {
public:
    std::string Interface() const { return interface_; }
    mcs::IpV4Address FixedAddress() const { return fixed_address_; }
    mcs::IpV4Address Gateway() const { return gateway_; }

private:
    std::string interface_;
    mcs::IpV4Address fixed_address_;
    mcs::IpV4Address gateway_;

    friend class DhcpLeaseParser;
};

std::ostream& operator<<(std::ostream& out, const DhcpLeaseInfo &lease);

class DhcpLeaseParser {
public:
    static std::vector<DhcpLeaseInfo> FromFile(const std::string &path);
};
} // w11tng

#endif
