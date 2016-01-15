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

#include <map>
#include <vector>
#include <string>

#include <mcs/utils.h>

#include <w11tng/config.h>

#include "dhcplistenerstub.h"

std::vector<std::string> ignore_variables = { "PATH", "SHLVL", "_", "PWD", "dhc_dbus" };

int main(int argc, char **argv) {
    char **item;
    std::map<std::string,std::string> properties;

    printf("test\n");

    for (item = environ; *item; item++) {
        auto parts = mcs::Utils::StringSplit(std::string(*item), '=');

        bool should_ignore = false;
        for (auto ignore : ignore_variables) {
            if (ignore == parts[0]) {
                should_ignore = true;
                break;
            }
        }

        if (should_ignore)
            continue;

        properties[parts[0]] = parts[1];

        MCS_DEBUG("%s=%s", parts[0], parts[1]);
    }

    auto stub = w11tng::DhcpListenerStub::Create(w11tng::kDhcpPrivateSocketPath);

    stub->EmitEventSync(properties);

    return 0;
}
