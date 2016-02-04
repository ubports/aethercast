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
#include <gmock/gmock.h>

#include <common/glibhelpers.h>
#include <common/dbusfixture.h>
#include <common/dbusnameowner.h>

#include <mcs/utils.h>
#include <mcs/logger.h>

#include <w11tng/interfaceselector.h>

#include "interfaceskeleton.h"

namespace {
class InterfaceSelectorFixture : public ::testing::Test,
                        public mcs::testing::DBusFixture,
                        public mcs::testing::DBusNameOwner {
public:
    InterfaceSelectorFixture() :
        mcs::testing::DBusNameOwner("fi.w1.wpa_supplicant1") {
    }
};

class AvailableInterfaces {
public:
    AvailableInterfaces(unsigned int number, unsigned int with_p2p_support) :
        interface_counter_(0) {
        for (int i = 0; i < number; i++)
            interfaces_.push_back(CreateInterface(false));

        for (int i = 0; i < with_p2p_support; i++)
            interfaces_.push_back(CreateInterface(true));
    }

    std::vector<std::string> ObjectPaths() const {
        std::vector<std::string> object_paths;
        for (auto iface : interfaces_)
            object_paths.push_back(iface->ObjectPath());
        return object_paths;
    }

private:
    w11tng::testing::InterfaceSkeleton::Ptr CreateInterface(bool p2p_support) {
        auto interface = std::make_shared<w11tng::testing::InterfaceSkeleton>(mcs::Utils::Sprintf("/interface_%d", interface_counter_));
        interface->SetDriver("nl80211");
        interface->SetIfname(mcs::Utils::Sprintf("wlan%d", interface_counter_));
        auto value = CreateCapabilities(p2p_support);
        interface->SetCapabilities(value);
        interface_counter_++;
        return interface;
    }

    GVariant* CreateCapabilities(bool p2p_support) {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

        GVariantBuilder modes_builder;
        g_variant_builder_init(&modes_builder, G_VARIANT_TYPE("as"));

        g_variant_builder_add(&modes_builder, "s", "infrastructure");
        g_variant_builder_add(&modes_builder, "s", "adhoc");
        if (p2p_support)
            g_variant_builder_add(&modes_builder, "s", "p2p");

        auto modes = g_variant_builder_end(&modes_builder);

        g_variant_builder_add(&builder, "{sv}", "Modes", modes);

        return g_variant_builder_end(&builder);
    }

private:
    std::vector<w11tng::testing::InterfaceSkeleton::Ptr> interfaces_;
    unsigned int interface_counter_;
};

class MockInterfaceSelectorDelegate : public w11tng::InterfaceSelector::Delegate {
public:
    MOCK_METHOD1(OnInterfaceSelectionDone, void(const std::string&));
};
}

TEST_F(InterfaceSelectorFixture, ProcessWithNoInterfaces) {
    auto selector = w11tng::InterfaceSelector::Create();
    EXPECT_TRUE(!!selector);

    auto delegate = std::make_shared<MockInterfaceSelectorDelegate>();

    EXPECT_CALL(*delegate, OnInterfaceSelectionDone(std::string(""))).Times(1);

    selector->SetDelegate(delegate);

    selector->Process(std::vector<std::string>{});

    mcs::testing::RunMainLoop(std::chrono::seconds{1});
}

TEST_F(InterfaceSelectorFixture, NoSelectableInterfaceAvailable) {
    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    auto selector = w11tng::InterfaceSelector::Create();
    EXPECT_TRUE(!!selector);

    auto delegate = std::make_shared<MockInterfaceSelectorDelegate>();

    EXPECT_CALL(*delegate, OnInterfaceSelectionDone(std::string(""))).Times(1);

    selector->SetDelegate(delegate);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    AvailableInterfaces ifaces(5, 0);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    selector->Process(ifaces.ObjectPaths());

    mcs::testing::RunMainLoop(std::chrono::seconds{1});
}

TEST_F(InterfaceSelectorFixture, MultipleSelectableInterfaces) {
    auto selector = w11tng::InterfaceSelector::Create();
    EXPECT_TRUE(!!selector);

    auto delegate = std::make_shared<MockInterfaceSelectorDelegate>();

    EXPECT_CALL(*delegate, OnInterfaceSelectionDone(std::string("/interface_3"))).Times(1);

    selector->SetDelegate(delegate);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    AvailableInterfaces ifaces(2, 2);

    mcs::testing::RunMainLoop(std::chrono::seconds{1});

    selector->Process(ifaces.ObjectPaths());

    mcs::testing::RunMainLoop(std::chrono::seconds{1});
}
