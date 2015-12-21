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

#include <glib.h>
#include <gtest/gtest.h>

#include <w11t/message.h>

TEST(Message, Ctor) {
    w11t::Message m = w11t::Message::CreateRequest("P2P_CONNECT");
    EXPECT_TRUE(m.ItsType() == w11t::Message::Type::kRequest);
    EXPECT_TRUE(m.Name() == std::string("P2P_CONNECT"));
}

TEST(Message, ReadAndSkip) {
    w11t::Message m = w11t::Message::Parse("<3>P2P-GROUP-STARTED p2p0 GO ssid=\"DIRECT-hB\" freq=2412 go_dev_addr=4e:74:03:64:95:a7");

    struct P2PGroupStarted {
        std::string interface_name;
        std::string type;
        w11t::Named<std::string> ssid{"ssid"};
        w11t::Named<std::uint32_t> freq{"freq"};
        w11t::Named<std::string> go_dev_addr{"go_dev_addr"};
    } ev;

    std::string key;

    EXPECT_STREQ(m.Name().c_str(), "P2P-GROUP-STARTED");
    EXPECT_NO_THROW(m.Read(ev.interface_name, ev.type, ev.ssid, ev.freq, ev.go_dev_addr));
    EXPECT_EQ(ev.interface_name, "p2p0");
    EXPECT_EQ(ev.ssid.Value(), "\"DIRECT-hB\"");
    EXPECT_EQ(ev.freq.Value(), 2412);
    EXPECT_EQ(ev.go_dev_addr.Value(), "4e:74:03:64:95:a7");

    m = w11t::Message::Parse("<3>P2P-GROUP-STARTED p2p0 GO -12 ssid=\"DIRECT-hB\" 45623 freq=2412 11223344 -42");

    ev = P2PGroupStarted{};

    uint32_t u32 = 0;
    int32_t i32 = 0;

    EXPECT_STREQ(m.Name().c_str(), "P2P-GROUP-STARTED");
    EXPECT_NO_THROW(m.Read(ev.interface_name, w11t::skip<std::string>(), w11t::skip<std::string>(), w11t::skip<std::string>(), ev.freq, u32, i32));
    EXPECT_EQ(ev.interface_name, "p2p0");
    EXPECT_EQ(ev.freq.Value(), 2412);
    EXPECT_EQ(u32, 11223344);
    EXPECT_EQ(i32, -42);

    m.Rewind();
    ev = P2PGroupStarted{};
    i32 = 0;

    EXPECT_NO_THROW(m.Read(ev.interface_name, w11t::skip<std::string>(), i32));
    EXPECT_EQ(ev.interface_name, "p2p0");
    EXPECT_EQ(i32, -12);

    m.Rewind();
    ev = P2PGroupStarted{};
    i32 = 0;

    EXPECT_NO_THROW(m.Read(w11t::skip<std::string>(), w11t::skip<std::string>(), w11t::skip<std::int32_t>(), ev.ssid, w11t::skip<std::int32_t>(), ev.freq));
    EXPECT_EQ(ev.ssid.Value(), "\"DIRECT-hB\"");
    EXPECT_EQ(ev.freq.Value(), 2412);
}

TEST(Message, Append) {
    w11t::Message m = w11t::Message::CreateRequest("P2P_CONNECT")
            << "string" << -42 << 1337 << w11t::Named<std::string>{"key", "value"};
    auto raw = m.Dump();
    EXPECT_STREQ(raw.c_str(), "P2P_CONNECT string -42 1337 key=value");

    m = w11t::Message::CreateRequest("P2P_FIND");
    int timeout = 30;
    m << timeout;
    raw = m.Dump();
    EXPECT_STREQ(raw.c_str(), "P2P_FIND 30");
}

TEST(Message, Sealing) {
    w11t::Message m = w11t::Message::CreateRequest("P2P_CONNECT");

    EXPECT_TRUE(!m.Sealed());
    m << "test1" << "test2";
    m.Seal();
    EXPECT_TRUE(m.Sealed());
    EXPECT_THROW(m << "foo", std::logic_error);
    EXPECT_STREQ(m.Raw().c_str(), "P2P_CONNECT test1 test2");
}

TEST(Message, CopyCtor) {
    w11t::Message m = w11t::Message::CreateRequest("P2P_CONNECT") << "test1" << "test2";
    EXPECT_TRUE(!m.Sealed());
    EXPECT_EQ(m.ItsType(), w11t::Message::Type::kRequest);
    EXPECT_STREQ(m.Name().c_str(), "P2P_CONNECT");
    m.Seal();
    EXPECT_TRUE(m.Sealed());
    EXPECT_STREQ(m.Raw().c_str(), "P2P_CONNECT test1 test2");

    w11t::Message m2 = m;
    EXPECT_TRUE(m.ItsType() == w11t::Message::Type::kRequest);
    EXPECT_STREQ(m.Name().c_str(), "P2P_CONNECT");
    EXPECT_TRUE(m.Sealed());
    EXPECT_STREQ(m.Raw().c_str(), "P2P_CONNECT test1 test2");
}

TEST(Message, OkFail) {
    w11t::Message m = w11t::Message::Parse("OK");
    EXPECT_TRUE(m.IsOk());

    m = w11t::Message::Parse("FAIL");
    EXPECT_TRUE(m.IsFail());
}
