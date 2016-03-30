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

#include <arpa/inet.h>

#include "w11tng/informationelement.h"

TEST(InformationElement, SourceWithAvailableSession) {
    w11tng::InformationElement ie;
    auto sub_element = new_subelement(w11tng::kDeviceInformation);
    const auto dev_info = reinterpret_cast<w11tng::DeviceInformationSubelement*>(sub_element);

    dev_info->session_management_control_port = htons(7236);
    dev_info->maximum_throughput = htons(50);
    dev_info->field1.device_type = w11tng::kSource;
    dev_info->field1.session_availability = true;
    ie.add_subelement(sub_element);

    auto ie_data = ie.serialize();

    uint8_t expected_bytes[] = { 0x0, 0x0, 0x6, 0x0, 0x10, 0x1c, 0x44, 0x0, 0x32 };

    EXPECT_EQ(ie_data->length, 9);
    for (unsigned int n = 0; n < ie_data->length; n++)
        EXPECT_EQ(ie_data->bytes[n], expected_bytes[n]);
}

TEST(InformationElement, DualRole) {
    w11tng::InformationElement ie;
    auto sub_element = new_subelement(w11tng::kDeviceInformation);
    const auto dev_info = reinterpret_cast<w11tng::DeviceInformationSubelement*>(sub_element);

    dev_info->session_management_control_port = htons(7236);
    dev_info->maximum_throughput = htons(50);
    dev_info->field1.device_type = w11tng::kDualRole;
    dev_info->field1.session_availability = true;
    ie.add_subelement(sub_element);

    auto ie_data = ie.serialize();

    uint8_t expected_bytes[] = { 0x0, 0x0, 0x6, 0x0, 0x13, 0x1c, 0x44, 0x0, 0x32 };

    EXPECT_EQ(ie_data->length, 9);
    for (unsigned int n = 0; n < ie_data->length; n++)
        EXPECT_EQ(ie_data->bytes[n], expected_bytes[n]);
}
