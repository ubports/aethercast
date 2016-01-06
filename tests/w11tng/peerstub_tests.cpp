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

#include <core/posix/fork.h>

#include <mcs/keep_alive.h>
#include <w11tng/peerstub.h>

#include <common/dbusfixture.h>

#include <mcs/logger.h>

namespace {
static void RunMainLoop(const std::chrono::seconds &seconds) {
    std::shared_ptr<GMainLoop> loop(g_main_loop_new(nullptr, false), &g_main_loop_unref);
    g_timeout_add_seconds(seconds.count(), [](gpointer user_data) {
        auto loop = static_cast<mcs::SharedKeepAlive<GMainLoop>*>(user_data)->ShouldDie();
        g_main_loop_quit(loop.get());
        return FALSE;
    }, new mcs::SharedKeepAlive<GMainLoop>{loop});
    g_main_loop_run(loop.get());
}

class PeerSkeleton {
public:
    PeerSkeleton(const std::string &object_path) :
        skeleton_(mcs::make_shared_gobject(wpa_supplicant_peer_skeleton_new())) {

        GError *error = nullptr;
        bus_connection_ = mcs::make_shared_gobject(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
        if (!bus_connection_) {
            g_error_free(error);
            return;
        }

        SetName("");
        SetAddress(std::vector<uint8_t>{});

        g_bus_own_name(G_BUS_TYPE_SYSTEM, "fi.w1.wpa_supplicant1", G_BUS_NAME_OWNER_FLAGS_NONE,
                       nullptr, nullptr, nullptr, nullptr, nullptr);

        g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(skeleton_.get()),
                                         bus_connection_.get(), object_path.c_str(), nullptr);
    }

    void SetAddress(const std::vector<uint8_t> &address) {
        auto value = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, address.size() == 0 ? nullptr : &address[0], address.size(), 1);
        wpa_supplicant_peer_set_device_address(skeleton_.get(), value);
    }

    void SetName(const std::string &name) {
        wpa_supplicant_peer_set_device_name(skeleton_.get(), name.c_str());
    }

private:
    mcs::SharedGObject<GDBusConnection> bus_connection_;
    mcs::SharedGObject<WpaSupplicantPeer> skeleton_;
};
}

class PeerStubFixture : public ::testing::Test,
                        public mcs::testing::DBusFixture {
};

class MockPeerDelegate : public w11tng::PeerStub::Delegate {
public:
    MOCK_METHOD0(OnPeerReady, void());
    MOCK_METHOD0(OnPeerChanged, void());
};

TEST_F(PeerStubFixture, ConstructionAndProperties) {
    auto delegate = std::make_shared<MockPeerDelegate>();

    EXPECT_CALL(*delegate, OnPeerReady()).Times(1);
    EXPECT_CALL(*delegate, OnPeerChanged()).Times(4);

    auto skeleton = std::make_shared<PeerSkeleton>("/peer_1");

    auto stub = w11tng::PeerStub::Create("/peer_1");
    EXPECT_TRUE(!!stub);

    stub->SetDelegate(delegate);

    RunMainLoop(std::chrono::seconds{1});

    EXPECT_EQ(stub->Address(), "00:00:00:00:00:00");
    EXPECT_EQ(stub->Name(), std::string(""));

    skeleton->SetAddress(std::vector<uint8_t>{ 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff });
    skeleton->SetName("Test Peer");

    RunMainLoop(std::chrono::seconds{5});

    EXPECT_EQ(stub->Address(), "aa:bb:cc:dd:ee:ff");
    EXPECT_EQ(stub->Name(), std::string("Test Peer"));

    MCS_DEBUG("done");
}
