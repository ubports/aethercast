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

#include <cstdint>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <common/glibhelpers.h>
#include <common/dbusfixture.h>
#include <common/dbusnameowner.h>

#include <w11tng/p2pdevicestub.h>

#include "p2pdeviceskeleton.h"

namespace {
class P2PDeviceStubFixture : public ::testing::Test,
                        public mcs::testing::DBusFixture,
                        public mcs::testing::DBusNameOwner {
public:
    P2PDeviceStubFixture() :
        mcs::testing::DBusNameOwner("fi.w1.wpa_supplicant1") {
    }
};
}

TEST_F(P2PDeviceStubFixture, Dummy) {
}
