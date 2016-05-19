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

#include <gtest/gtest.h>

#include <fstream>

#include <boost/filesystem.hpp>

#include "ac/utils.h"
#include "w11tng/dhcpleaseparser.h"

namespace {
std::vector<std::string> leases_multiple_entries = {
    "# Test comment",
    "lease {",
    "    interface p2p0;",
    "    fixed-address 192.168.7.5;",
    "    option routers 192.168.7.1;",
    "    option host-name \"Test Host\";",
    "}",
    "lease {",
    "    interface wlan0;",
    "    fixed-address 10.10.7.5;",
    "    option routers 10.10.7.1;",
    "    option host-name \"Test Host\";",
    "}",
};

std::vector<std::string> leases_with_invalid_ip = {
    "# Test comment",
    "lease {",
    "    interface p2p0;",
    "    fixed-address 192.168.7.x;",
    "    option routers 192.168.x.1;",
    "    option host-name \"Test Host\";",
    "}",
};

std::string CreateLeaseFile(const std::vector<std::string> &content) {
    auto path = ac::Utils::Sprintf("%s/test-leases-%s",
                                    boost::filesystem::temp_directory_path().string(),
                                    boost::filesystem::unique_path().string());
    ac::Utils::CreateFile(path);

    std::fstream file(path);
    for (auto line : content)
        file << line << std::endl;
    file.close();

    return path;
}
}

TEST(DhcpLeaseParser, ReadMultipleLeases) {
    auto lease_path = CreateLeaseFile(leases_multiple_entries);
    auto leases = w11tng::DhcpLeaseParser::FromFile(lease_path);

    EXPECT_EQ(leases.size(), 2);

    EXPECT_EQ(leases[0].Interface(), "p2p0");
    EXPECT_EQ(leases[0].FixedAddress().to_string(), "192.168.7.5");
    EXPECT_EQ(leases[0].Gateway().to_string(), "192.168.7.1");

    EXPECT_EQ(leases[1].Interface(), "wlan0");
    EXPECT_EQ(leases[1].FixedAddress().to_string(), "10.10.7.5");
    EXPECT_EQ(leases[1].Gateway().to_string(), "10.10.7.1");

    ::unlink(lease_path.c_str());
}

TEST(DhcpLeaseParser, InvalidIpAddresses) {
    auto lease_path = CreateLeaseFile(leases_with_invalid_ip);
    auto leases = w11tng::DhcpLeaseParser::FromFile(lease_path);

    EXPECT_EQ(leases.size(), 0);

    ::unlink(lease_path.c_str());
}
