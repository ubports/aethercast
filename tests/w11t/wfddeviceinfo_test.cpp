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

#include <w11t/wfddeviceinfo.h>

TEST(WfdDeviceInfo, FromStringWithValidInput) {
    auto info = w11t::WfdDeviceInfo::Parse("0x00111c440032");
    EXPECT_TRUE(info.IsSupported());
    EXPECT_TRUE(info.IsSupportedSink());
    EXPECT_EQ(info.ctrl_port_, 7236);
    EXPECT_EQ(info.max_tput_, 50);
}

TEST(WfdDeviceInfo, FromStringWithInvalidInput) {
    auto info = w11t::WfdDeviceInfo::Parse("0xa1231sdas12312131232134235243rasasdsa");
    EXPECT_TRUE(!info.IsSupported());
    EXPECT_TRUE(!info.IsSupportedSink());
    EXPECT_EQ(info.ctrl_port_, 0);
    EXPECT_EQ(info.max_tput_, 0);

    EXPECT_THROW(w11t::WfdDeviceInfo::Parse("0xabcdefghijkl"), std::invalid_argument);
}
