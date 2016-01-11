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

#include <gtest/gtest.h>

#include <mcs/networkmanagerfactory.h>
#include <w11t/networkmanager.h>
#include <w11tng/networkmanager.h>

struct NetworkManagerFactoryFixture : public ::testing::Test {
    template <typename T>
    void CheckManagerCreation(const std::string &type_name) {
        setenv("AETHERCAST_NETWORK_MANAGER", type_name.c_str(), 1);
        EXPECT_TRUE((std::dynamic_pointer_cast<T>(mcs::NetworkManagerFactory::Create()) ? true : false));
    }
};

TEST_F(NetworkManagerFactoryFixture, InvalidType) {
    CheckManagerCreation<mcs::NullNetworkManager>("foobar");
}

TEST_F(NetworkManagerFactoryFixture, DefaultType) {
    CheckManagerCreation<w11tng::NetworkManager>("");
}

TEST_F(NetworkManagerFactoryFixture, W11tTypeCreation) {
    CheckManagerCreation<w11t::NetworkManager>("w11t");
    CheckManagerCreation<mcs::NullNetworkManager>("w11t123");
    CheckManagerCreation<mcs::NullNetworkManager>("123w11t123");
    CheckManagerCreation<mcs::NullNetworkManager>("123w11t");
}

TEST_F(NetworkManagerFactoryFixture, W11tngTypeCreation) {
    CheckManagerCreation<w11tng::NetworkManager>("w11tng");
    CheckManagerCreation<mcs::NullNetworkManager>("w11tng123");
    CheckManagerCreation<mcs::NullNetworkManager>("123w11tng123");
    CheckManagerCreation<mcs::NullNetworkManager>("123w11tng");
}
