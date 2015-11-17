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

#include "wpasupplicantmessage.h"

void test_wpasupplicant_message_ctor(void) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");
    g_assert(m.Type() == kRequest);
    g_assert(m.Name() == std::string("P2P_CONNECT"));
}

void test_wpasupplicant_message_read_skip(void) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRaw("<3>P2P-GROUP-STARTED p2p0 GO ssid=\"DIRECT-hB\" freq=2412 go_dev_addr=4e:74:03:64:95:a7");

    char *interface_name = nullptr;
    char *type = nullptr;
    char *ssid = nullptr;
    char *freq = nullptr;
    char *go_dev_addr = nullptr;

    g_assert(m.Name().compare("P2P-GROUP-STARTED") == 0);
    g_assert(m.Read("sseee", &interface_name, &type, &ssid, &freq, &go_dev_addr));
    g_assert(g_strcmp0(interface_name, "p2p0") == 0);
    g_assert(g_strcmp0(ssid, "\"DIRECT-hB\"") == 0);
    g_assert(g_strcmp0(freq, "2412") == 0);
    g_assert(g_strcmp0(go_dev_addr, "4e:74:03:64:95:a7") == 0);

    m = WpaSupplicantMessage::CreateRaw("<3>P2P-GROUP-STARTED p2p0 GO -12 ssid=\"DIRECT-hB\" 45623 freq=2412 11223344 -42");

    interface_name = nullptr;
    freq = nullptr;

    uint32_t u32 = 0;
    int32_t i32 = 0;

    g_assert(m.Name().compare("P2P-GROUP-STARTED") == 0);
    g_assert(m.Read("s", &interface_name));
    g_assert(g_strcmp0(interface_name, "p2p0") == 0);
    g_assert(m.Skip("sieu"));
    g_assert(m.Read("e", &freq));
    g_assert(g_strcmp0(freq, "2412") == 0);
    g_assert(m.Read("u", &u32));
    g_assert(u32 == 11223344);
    g_assert(m.Read("i", &i32));
    g_assert(i32 == -42);

    m.Rewind();

    interface_name = nullptr;
    i32 = 0;

    g_assert(m.Read("s", &interface_name));
    g_assert(g_strcmp0(interface_name, "p2p0") == 0);
    g_assert(m.Skip("s"));
    g_assert(m.Read("i", &i32));
    g_assert(i32 == -12);
}

void test_wpasupplicant_message_append(void) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");

    g_assert(m.Append("siue", "string", -42, 1337, "key", "value"));
    auto raw = m.Dump();
    g_assert(raw.compare("P2P_CONNECT string -42 1337 key=value") == 0);

    m = WpaSupplicantMessage::CreateRequest("P2P_FIND");
    int timeout = 30;
    g_assert(m.Append("i", timeout));
    raw = m.Dump();
    g_assert(raw.compare("P2P_FIND 30") == 0);
}

void test_wpasupplicant_message_sealing() {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");

    g_assert(!m.Sealed());
    g_assert(m.Append("ss", "test1", "test2"));
    m.Seal();
    g_assert(m.Sealed());
    g_assert(!m.Append("s", "foo"));
    g_assert(m.Raw().compare("P2P_CONNECT test1 test2") == 0);
}

void test_wpasupplicant_message_copy_ctor(void) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRequest("P2P_CONNECT");
    g_assert(m.Append("ss", "test1", "test2"));
    g_assert(!m.Sealed());
    g_assert(m.Type() == kRequest);
    g_assert(m.Name() == std::string("P2P_CONNECT"));
    m.Seal();
    g_assert(m.Sealed());
    g_assert(m.Raw().compare("P2P_CONNECT test1 test2") == 0);

    WpaSupplicantMessage m2 = m;
    g_assert(m.Type() == kRequest);
    g_assert(m.Name() == std::string("P2P_CONNECT"));
    g_assert(m.Sealed());
    g_assert(m.Raw().compare("P2P_CONNECT test1 test2") == 0);
}

void test_wpasupplicant_message_ok_fail(void) {
    WpaSupplicantMessage m = WpaSupplicantMessage::CreateRaw("OK");
    g_assert(m.IsOk());

    m = WpaSupplicantMessage::CreateRaw("FAIL");
    g_assert(m.IsFail());
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/wpasupplicant/message/ctor", test_wpasupplicant_message_ctor);
    g_test_add_func("/wpasupplicant/message/read_skip", test_wpasupplicant_message_read_skip);
    g_test_add_func("/wpasupplicant/message/append", test_wpasupplicant_message_append);
    g_test_add_func("/wpasupplicant/message/sealing", test_wpasupplicant_message_sealing);
    g_test_add_func("/wpasupplicant/message/copy-ctor", test_wpasupplicant_message_copy_ctor);
    g_test_add_func("/wpasupplicant/message/ok-fail", test_wpasupplicant_message_ok_fail);

    return g_test_run();
}
