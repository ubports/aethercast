/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */
#ifndef CORE_DBUS_FIXTURE_H_
#define CORE_DBUS_FIXTURE_H_

#include <chrono>
#include <memory>
#include <string>

namespace mcs
{
namespace testing
{

/**
 * @brief The Fixture class provides private session and system bus instances for testing purposes.
 */
class DBusFixture
{
public:
    /** @brief Fractional seconds. */
    typedef std::chrono::duration<double> Seconds;

    /**
      * @brief default_daemon_timeout after which the dbus daemons will be killed.
      */
    static Seconds& default_daemon_timeout();

    /**
     * @brief Constructs a fixture instance with the two given configuration files.
     *
     * Any test running within the scope of this fixture will access the private
     * system-bus instance setup by this class.
     */
    DBusFixture();

    virtual ~DBusFixture();

    /**
     * @brief session_bus_address returns the address of the private session bus instance.
     */
    std::string system_bus_address();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

}
}

#endif // CORE_DBUS_FIXTURE_H_
