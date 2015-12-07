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

#include <csignal>
#include <cstdio>
#include <cstdint>

#include <sys/prctl.h>

#include <glib.h>
#include <glib-unix.h>
#include <gst/gst.h>

#include <chrono>

#include "keep_alive.h"
#include "miracastservice.h"
#include "miracastserviceadapter.h"
#include "wpasupplicantnetworkmanager.h"
#include "wfddeviceinfo.h"
#include "types.h"
#include "logging.h"

namespace {
// TODO(morphis, tvoss): Expose the port as a construction-time parameter.
const std::uint16_t kMiracastDefaultRtspCtrlPort{7236};
const std::chrono::milliseconds kStateIdleTimeout{1000};
}
namespace mcs {
MiracastService::MainOptions MiracastService::MainOptions::FromCommandLine(int argc, char** argv) {
    static gboolean option_debug{FALSE};
    static gboolean option_version{FALSE};

    static GOptionEntry options[] = {
        { "debug", 'd', 0, G_OPTION_ARG_NONE, &option_debug, "Enable debugging mode", nullptr },
        { "version", 'v', 0, G_OPTION_ARG_NONE, &option_version, "Show version information and exit", nullptr },
        { NULL },
    };

    std::shared_ptr<GOptionContext> context{g_option_context_new(nullptr), [](GOptionContext *ctxt) { g_option_context_free(ctxt); }};
    GError *error = nullptr;

    g_option_context_add_main_entries(context.get(), options, NULL);

    if (!g_option_context_parse(context.get(), &argc, &argv, &error)) {
        if (error) {
            g_printerr("%s\n", error->message);
            g_error_free(error);
        } else
            g_printerr("An unknown error occurred\n");
        exit(1);
    }

    return MainOptions{option_debug == TRUE, option_version == TRUE};
}

int MiracastService::Main(const MiracastService::MainOptions &options) {
    if (options.print_version) {
        std::printf("%d.%d\n", MiracastService::kVersionMajor, MiracastService::kVersionMinor);
        return 0;
    }

    if (options.debug) {
        mcs::InitLogging(kDebug);
        mcs::Info("Debugging enabled");
    }

    struct Runtime {
        static int OnSignalRaised(gpointer user_data) {
            auto thiz = static_cast<Runtime*>(user_data);
            g_main_loop_quit(thiz->ml);

            return 0;
        }

        Runtime() {
            // We do not have to use a KeepAlive<Scope> here as
            // a Runtime instance controls the lifetime of signal
            // emissions.
            g_unix_signal_add(SIGINT, OnSignalRaised, this);
            g_unix_signal_add(SIGTERM, OnSignalRaised, this);

            // Become a reaper of all our children
            if (prctl(PR_SET_CHILD_SUBREAPER, 1) < 0)
                g_warning("Failed to make us a subreaper of our children");
        }

        ~Runtime() {
            g_main_loop_unref(ml);
            gst_deinit();
        }

        void Run() {
            g_main_loop_run(ml);
        }

        GMainLoop *ml = g_main_loop_new(nullptr, FALSE);
        bool is_gst_initialized = gst_init_check(nullptr, nullptr, nullptr);
    } rt;

    auto service = mcs::MiracastService::Create();
    auto mcsa = mcs::MiracastServiceAdapter::create(service);

    rt.Run();

    return 0;
}

std::shared_ptr<MiracastService> MiracastService::Create() {
    auto sp = std::shared_ptr<MiracastService>{new MiracastService{}};
    return sp->FinalizeConstruction();
}

MiracastService::MiracastService() :
    network_manager_(new WpaSupplicantNetworkManager(this)),
    source_(nullptr),
    current_state_(kIdle),
    current_device_(nullptr),
    scan_timeout_source_(0) {
    network_manager_->Setup();
}

std::shared_ptr<MiracastService> MiracastService::FinalizeConstruction() {
    return shared_from_this();
}

MiracastService::~MiracastService() {
    if (scan_timeout_source_ > 0)
        g_source_remove(scan_timeout_source_);
}

void MiracastService::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

void MiracastService::ResetDelegate() {
    delegate_.reset();
}

NetworkDeviceState MiracastService::State() const {
    return current_state_;
}

void MiracastService::OnClientDisconnected() {
    AdvanceState(kFailure);
    source_.reset();
    current_device_.reset();
}

void MiracastService::AdvanceState(NetworkDeviceState new_state) {
    IpV4Address address;

    mcs::Debug("new state %d current state %d",
               mcs::NetworkDevice::StateToStr(new_state).c_str(),
               mcs::NetworkDevice::StateToStr(current_state_).c_str());

    switch (new_state) {
    case kAssociation:
        break;

    case kConnected:
        address = network_manager_->LocalAddress();

        source_ = MiracastSourceManager::Create(address, kMiracastDefaultRtspCtrlPort);

        FinishConnectAttempt();

        break;

    case kFailure:
        FinishConnectAttempt(kErrorFailed);

    case kDisconnected:
        source_.reset();
        current_device_.reset();

        StartIdleTimer();
        break;

    case kIdle:
        break;

    default:
        break;
    }

    current_state_ = new_state;
    if (auto sp = delegate_.lock())
        sp->OnStateChanged(current_state_);
}

void MiracastService::OnDeviceStateChanged(const NetworkDevice::Ptr &device) {
    mcs::Debug("Device state changed: address %s new state %s",
               device->Address().c_str(),
               mcs::NetworkDevice::StateToStr(device->State()).c_str());

    if (device != current_device_)
        return;

    AdvanceState(device->State());
}

void MiracastService::OnDeviceChanged(const NetworkDevice::Ptr &device) {
    mcs::Debug("device %s", device->Address().c_str());
    if (auto sp = delegate_.lock())
        sp->OnDeviceChanged(device);
}

void MiracastService::OnDeviceFound(const NetworkDevice::Ptr &device) {
    if (auto sp = delegate_.lock())
        sp->OnDeviceFound(device);
}

void MiracastService::OnDeviceLost(const NetworkDevice::Ptr &device) {
    if (auto sp = delegate_.lock())
        sp->OnDeviceLost(device);
}

gboolean MiracastService::OnIdleTimer(gpointer user_data) {
    auto inst = static_cast<SharedKeepAlive<MiracastService>*>(user_data)->ShouldDie();
    inst->AdvanceState(kIdle);
    return FALSE;
}

void MiracastService::StartIdleTimer() {
    g_timeout_add(kStateIdleTimeout.count(), &MiracastService::OnIdleTimer, new SharedKeepAlive<MiracastService>{shared_from_this()});
}

void MiracastService::FinishConnectAttempt(mcs::Error error) {
    if (connect_callback_)
        connect_callback_(error);

    connect_callback_ = nullptr;
}

void MiracastService::Connect(const NetworkDevice::Ptr &device, ResultCallback callback) {
    if (current_device_) {
        callback(kErrorAlready);
        return;
    }

    if (!device) {
        callback(kErrorParamInvalid);
        return;
    }

    if (!network_manager_->Connect(device)) {
        callback(kErrorFailed);
        return;
    }

    current_device_ = device;
    connect_callback_ = callback;
}

void MiracastService::Disconnect(const NetworkDevice::Ptr &device, ResultCallback callback) {
    if (!current_device_ || !device) {
        callback(kErrorParamInvalid);
        return;
    }

    if (!network_manager_->Disconnect(device)) {
        callback(kErrorFailed);
        return;
    }

    callback(kErrorNone);
}

gboolean MiracastService::OnScanTimeout(gpointer user_data) {
    auto inst = static_cast<MiracastService*>(user_data);

    inst->network_manager_->StopScan();

    if (inst->current_scan_callback_) {
        inst->current_scan_callback_(kErrorNone);
        inst->current_scan_callback_ = nullptr;
    }

    inst->scan_timeout_source_ = 0;

    return FALSE;
}

void MiracastService::Scan(ResultCallback callback, const std::chrono::seconds &timeout) {
    if (scan_timeout_source_ > 0 || network_manager_->Scanning()) {
        callback(kErrorAlready);
        return;
    }

    scan_timeout_source_ = g_timeout_add_seconds(timeout.count(), &MiracastService::OnScanTimeout, this);
    current_scan_callback_ = callback;

    network_manager_->Scan();
}
} // namespace miracast
