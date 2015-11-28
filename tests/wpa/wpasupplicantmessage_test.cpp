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

#include "wpa/wpasupplicantmessage.h"

TEST(WpaSupplicantMessage, Ctor) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");
    EXPECT_TRUE(m.Type() == kRequest);
    EXPECT_TRUE(m.Name() == std::string("P2P_CONNECT"));
}

TEST(WpaSupplicantMessage, ReadAndSkip) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRaw("<3>P2P-GROUP-STARTED p2p0 GO ssid=\"DIRECT-hB\" freq=2412 go_dev_addr=4e:74:03:64:95:a7");

    char *interface_name = nullptr;
    char *type = nullptr;
    char *ssid = nullptr;
    char *freq = nullptr;
    char *go_dev_addr = nullptr;

    EXPECT_TRUE(m.Name().compare("P2P-GROUP-STARTED") == 0);
    EXPECT_TRUE(m.Read("sseee", &interface_name, &type, &ssid, &freq, &go_dev_addr));
    EXPECT_TRUE(g_strcmp0(interface_name, "p2p0") == 0);
    EXPECT_TRUE(g_strcmp0(ssid, "\"DIRECT-hB\"") == 0);
    EXPECT_TRUE(g_strcmp0(freq, "2412") == 0);
    EXPECT_TRUE(g_strcmp0(go_dev_addr, "4e:74:03:64:95:a7") == 0);

    m = WpaSupplicantMessage::CreateRaw("<3>P2P-GROUP-STARTED p2p0 GO -12 ssid=\"DIRECT-hB\" 45623 freq=2412 11223344 -42");

    interface_name = nullptr;
    freq = nullptr;

    uint32_t u32 = 0;
    int32_t i32 = 0;

    EXPECT_TRUE(m.Name().compare("P2P-GROUP-STARTED") == 0);
    EXPECT_TRUE(m.Read("s", &interface_name));
    EXPECT_TRUE(g_strcmp0(interface_name, "p2p0") == 0);
    EXPECT_TRUE(m.Skip("sieu"));
    EXPECT_TRUE(m.Read("e", &freq));
    EXPECT_TRUE(g_strcmp0(freq, "2412") == 0);
    EXPECT_TRUE(m.Read("u", &u32));
    EXPECT_TRUE(u32 == 11223344);
    EXPECT_TRUE(m.Read("i", &i32));
    EXPECT_TRUE(i32 == -42);

    m.Rewind();

    interface_name = nullptr;
    i32 = 0;

    EXPECT_TRUE(m.Read("s", &interface_name));
    EXPECT_TRUE(g_strcmp0(interface_name, "p2p0") == 0);
    EXPECT_TRUE(m.Skip("s"));
    EXPECT_TRUE(m.Read("i", &i32));
    EXPECT_TRUE(i32 == -12);

    ssid = nullptr;
    i32 = 0;

    EXPECT_TRUE(m.ReadDictEntry("ssid", 's', &ssid));
    EXPECT_TRUE(g_strcmp0(ssid, "\"DIRECT-hB\"") == 0);
    EXPECT_TRUE(m.ReadDictEntry("freq", 'i', &i32));
    EXPECT_TRUE(i32 == 2412);
}

TEST(WpaSupplicantMessage, Append) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");

    EXPECT_TRUE(m.Append("siue", "string", -42, 1337, "key", "value"));
    auto raw = m.Dump();
    EXPECT_TRUE(raw.compare("P2P_CONNECT string -42 1337 key=value") == 0);

    m = WpaSupplicantMessage::CreateRequest("P2P_FIND");
    int timeout = 30;
    EXPECT_TRUE(m.Append("i", timeout));
    raw = m.Dump();
    EXPECT_TRUE(raw.compare("P2P_FIND 30") == 0);
}

TEST(WpaSupplicantMessage, Sealing) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");

    EXPECT_TRUE(!m.Sealed());
    EXPECT_TRUE(m.Append("ss", "test1", "test2"));
    m.Seal();
    EXPECT_TRUE(m.Sealed());
    EXPECT_TRUE(!m.Append("s", "foo"));
    EXPECT_TRUE(m.Raw().compare("P2P_CONNECT test1 test2") == 0);
}

void test_wpasupplicant_message_copy_ctor(void) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");
    EXPECT_TRUE(m.Append("ss", "test1", "test2"));
    EXPECT_TRUE(!m.Sealed());
    EXPECT_TRUE(m.Type() == kRequest);
    EXPECT_TRUE(m.Name() == std::string("P2P_CONNECT"));
    m.Seal();
    EXPECT_TRUE(m.Sealed());
    EXPECT_TRUE(m.Raw().compare("P2P_CONNECT test1 test2") == 0);

    WpaSupplicantMessage m2 = m;
    EXPECT_TRUE(m.Type() == kRequest);
    EXPECT_TRUE(m.Name() == std::string("P2P_CONNECT"));
    EXPECT_TRUE(m.Sealed());
    EXPECT_TRUE(m.Raw().compare("P2P_CONNECT test1 test2") == 0);
}

void test_wpasupplicant_message_ok_fail(void) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRaw("OK");
    EXPECT_TRUE(m.IsOk());

    m = WpaSupplicantMessage::CreateRaw("FAIL");
    EXPECT_TRUE(m.IsFail());
}
