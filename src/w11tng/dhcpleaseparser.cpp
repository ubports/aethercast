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

#include <fstream>
#include <sstream>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include <mcs/utils.h>
#include <mcs/logger.h>

#include <iostream>

#include "dhcpleaseparser.h"

namespace w11tng {
std::vector<DhcpLeaseInfo> DhcpLeaseParser::FromFile(const std::string &path) {
    std::vector<DhcpLeaseInfo> leases;

    if (!boost::filesystem::is_regular_file(path))
        return {};

    std::ifstream stream(path);
    std::string line;
    DhcpLeaseInfo current_lease;
    bool in_lease = false;
    while(std::getline(stream, line, '\n')) {
        boost::algorithm::trim(line);

        if (line.length() == 0)
            continue;

        if (mcs::Utils::StringStartsWith(line, "#"))
            continue;

        if (!in_lease && mcs::Utils::StringStartsWith(line, "lease")) {
            in_lease = true;

            auto parts = mcs::Utils::StringSplit(line, ' ');
            if (parts.size() == 3) {
                // we have a client lease so extract the address
                current_lease.fixed_address_ = mcs::IpV4Address::from_string(parts[1]);
            }

            continue;
        }

        if (in_lease && mcs::Utils::StringStartsWith(line, "}")) {
            leases.push_back(current_lease);
            current_lease = DhcpLeaseInfo();
            in_lease = false;
            continue;
        }

        if (!in_lease)
            continue;

        auto colon_pos = line.find_last_of(';');
        if (colon_pos != std::string::npos)
            line = line.substr(0, colon_pos);

        std::map<std::string,std::string> options;

        try {
            boost::tokenizer<boost::escaped_list_separator<char>> tokenizer(line, boost::escaped_list_separator<char>('\\', ' ', '\"'));
            int n = 0;
            std::string key;
            for (auto token = tokenizer.begin(); token != tokenizer.end(); ++token) {
                if (*token == "option")
                    continue;
                if (n % 2 == 0)
                    key = *token;
                else
                    options[key] = *token;
                n++;
            }
        }
        catch (...) {
            // Ignore any error from the tokenizer and just conintue
        }

        try {
            if (options.find("interface") != options.end())
                current_lease.interface_ = options["interface"];
            else if (options.find("fixed-address") != options.end())
                current_lease.fixed_address_ = mcs::IpV4Address::from_string(options["fixed-address"]);
            else if (options.find("routers") != options.end())
                current_lease.gateway_ = mcs::IpV4Address::from_string(options["routers"]);

            // As we're running in a network with only two peers the DHCP
            // service can be always taken as the gateway for us.
            if (current_lease.gateway_.is_unspecified() &&
                    options.find("dhcp-server-identifier") != options.end())
                current_lease.gateway_ = mcs::IpV4Address::from_string(options["dhcp-server-identifier"]);
        }
        catch (...) {
            // If we get an exception here its most likely due to one of
            // the parsed IP addresses being wrong. In that case we simply
            // return with an empty result list.
            return {};
        }
    }

    return leases;
}

std::ostream& operator<<(std::ostream& out, const DhcpLeaseInfo &lease) {
    return out << "interface " << lease.Interface()
               << " address " << lease.FixedAddress()
               << " gateway " << lease.Gateway();
}

}
