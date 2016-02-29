/*
 *  Copyright (C) 2016 Canonical, Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <glib.h>
#include <glib-unix.h>

#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <string>
#include <iostream>
#include <chrono>

#include <boost/program_options.hpp>

#include <mcs/logger.h>
#include <mcs/initgstreameronce.h>

#include <mcs/video/statistics.h>

#include "simplesource.h"

static GMainLoop *main_loop = nullptr;

namespace {
const std::chrono::seconds kShutdownGracePreriod{1};
const std::int16_t kProcessPriorityUrgentDisplay{-8};
mcs::tools::SimpleSource::Ptr source;

static gboolean OnSignalRaised(gpointer user_data) {
    MCS_DEBUG("Exiting");

    g_timeout_add_seconds(kShutdownGracePreriod.count(), [](gpointer user_data) {
        g_main_loop_quit(main_loop);
        return FALSE;
    }, nullptr);

    // This will bring down the whole pipeline and terminate everything
    // correctly.
    source.reset();

    // A second SIGTERM should really terminate us and also overlay
    // the grace period for a proper shutdown we're performing.
    return FALSE;
}
}

int main(int argc, char **argv) {
    std::string remote_address;
    int port = 0;
    bool debug = false;

#if 0
    mcs::InitGstreamerOnceOrThrow();
#endif

    g_unix_signal_add(SIGINT, OnSignalRaised, nullptr);
    g_unix_signal_add(SIGTERM, OnSignalRaised, nullptr);

    main_loop = g_main_loop_new(nullptr, FALSE);

    boost::program_options::options_description desc("Usage");
    desc.add_options()
        ("help,h", "displays this message")
        ("remote,r",
            boost::program_options::value<std::string>(&remote_address), "Remote host address")
        ("port,p",
            boost::program_options::value<int>(&port), "Port to use")
        ("debug,d",
            boost::program_options::bool_switch(&debug), "Enable verbose debug output");

    boost::program_options::variables_map vm;
    try {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
    }
    catch(boost::program_options::error& e) {
        std::cerr << e.what() << std::endl << std::endl;
        std::cerr << desc << std::endl;
        return EXIT_FAILURE;
    }

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    if (remote_address.length() == 0)
        throw std::runtime_error("Invalid address supplied");

    if (port == 0)
        throw std::runtime_error("Invalid or no port supplied");

    setpriority(PRIO_PROCESS, 0, kProcessPriorityUrgentDisplay);

    if (debug)
        mcs::Log().Init(mcs::Logger::Severity::kDebug);

    source = mcs::tools::SimpleSource::Create(remote_address, port);
    source->Start();

    g_main_loop_run(main_loop);

    g_main_loop_unref(main_loop);

    return EXIT_SUCCESS;
}
