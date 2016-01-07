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

#include <sstream>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include <core/posix/exec.h>
#include <core/posix/this_process.h>

#include "dbusfixture.h"

namespace
{
static constexpr const char* dbus_session_bus_address
{
    "DBUS_SESSION_BUS_ADDRESS"
};

static constexpr const char* dbus_system_bus_address
{
    "DBUS_SYSTEM_BUS_ADDRESS"
};

static constexpr const char* dbus_starter_address
{
    "DBUS_STARTER_ADDRESS"
};

static constexpr const char* dbus_starter_bus_type
{
    "DBUS_STARTER_BUS_TYPE"
};

boost::optional<boost::filesystem::path> find_executable(const boost::filesystem::path& name)
{
  if (name.is_absolute())
    return name;

  std::stringstream ss{core::posix::this_process::env::get("PATH")};
  std::string token;

  while (std::getline(ss, token, ':'))
  {
    auto path = boost::filesystem::path{token} / name;
    if (boost::filesystem::exists(path))
      return path;
  }

  return boost::optional<boost::filesystem::path>();
}

boost::filesystem::path dbus_daemon()
{
  if (auto path = find_executable("dbus-daemon"))
    return *path;

  throw std::runtime_error("Could not locate the dbus-daemon executable, please check your installation.");
}

boost::filesystem::path timeout()
{
  if (auto path = find_executable("timeout"))
    return *path;

  throw std::runtime_error("Could not locate the timeout executable, please check your installation.");
}
}

struct mcs::testing::DBusFixture::Private
{
    struct System
    {
        System()
        {
            std::vector<std::string> config
            {
                "<!DOCTYPE busconfig PUBLIC \"-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN\"",
                "\"http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd\">",
                "<busconfig>",
                "<keep_umask/>",
                "<type>system</type>",
                "<listen>unix:tmpdir=/tmp</listen>",
                "<standard_system_servicedirs/>",
                "<policy context=\"default\">",
                "<allow send_destination=\"*\" eavesdrop=\"true\"/>",
                "<allow eavesdrop=\"true\"/>",
                "<allow own=\"*\"/>",
                "</policy>",
                "</busconfig>"
            };

            auto config_path = boost::filesystem::temp_directory_path();
            config_path += "/mcs-dbus-system-";
            config_path += boost::filesystem::unique_path();

            std::ofstream file_stream;
            file_stream.open(config_path.string());
            for (auto line : config)
                file_stream << line << std::endl;
            file_stream.close();

            std::vector<std::string> argv
            {
                "--kill-after=5",
                std::to_string(mcs::testing::DBusFixture::default_daemon_timeout().count()),
                dbus_daemon().native(),
                "--config-file",
                config_path.string(),
                "--print-address"
            };

            std::map<std::string, std::string> env;
            core::posix::this_process::env::for_each([&env](const std::string& key, const std::string& value)
            {
                env.insert(std::make_pair(key, value));
            });

            daemon = core::posix::exec(
                        timeout().native(),
                        argv,
                        env,
                        core::posix::StandardStream::stdout);

            daemon.cout() >> address;

            if (address.empty())
                throw std::runtime_error("System: Could not read address of bus instance.");

            // We clean up the env prior to injecting the new addresses.
            std::error_code ec; // And just ignore all error codes.
            core::posix::this_process::env::unset(dbus_system_bus_address, ec);
            core::posix::this_process::env::set_or_throw(dbus_system_bus_address, address);
        }

        ~System()
        {
            std::error_code ec; // And just ignore all error codes.
            core::posix::this_process::env::unset(dbus_system_bus_address, ec);

            daemon.send_signal_or_throw(core::posix::Signal::sig_kill);
            daemon.wait_for(core::posix::wait::Flags::untraced);
        }

        core::posix::ChildProcess daemon = core::posix::ChildProcess::invalid();
        std::string address;
    } system;
};

mcs::testing::DBusFixture::Seconds& mcs::testing::DBusFixture::default_daemon_timeout()
{
    static mcs::testing::DBusFixture::Seconds instance{60};
    return instance;
}

mcs::testing::DBusFixture::DBusFixture()
    : d(new Private{Private::System{}})
{
}

mcs::testing::DBusFixture::~DBusFixture()
{
}

std::string mcs::testing::DBusFixture::system_bus_address()
{
    return d->system.address;
}
