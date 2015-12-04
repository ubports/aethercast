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

    auto service = mcs::MiracastService::create();
    auto mcsa = mcs::MiracastServiceAdapter::create(service);

    rt.Run();

    return 0;
}

std::shared_ptr<MiracastService> MiracastService::create() {
    auto sp = std::shared_ptr<MiracastService>{new MiracastService{}};
    return sp->FinalizeConstruction();
}

MiracastService::MiracastService() :
    network_manager_(new WpaSupplicantNetworkManager(this)),
    source_(MiracastSourceManager::create()),
    current_state_(kIdle),
    current_peer_(nullptr),
    scan_timeout_source_(0) {
    network_manager_->Setup();
}

std::shared_ptr<MiracastService> MiracastService::FinalizeConstruction() {
    auto sp = shared_from_this();
    source_->SetDelegate(sp);
    return sp;
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
    current_peer_.reset();
}

void MiracastService::AdvanceState(NetworkDeviceState new_state) {
    IpV4Address address;

    g_warning("AdvanceState newsstate %d current state %d", new_state, current_state_);

    switch (new_state) {
    case kAssociation:
        break;

    case kConnected:
        address = network_manager_->LocalAddress();
        source_->Setup(address, kMiracastDefaultRtspCtrlPort);

        FinishConnectAttempt(true);

        break;

    case kFailure:
        if (current_state_ == kAssociation ||
                current_state_ == kConfiguration)
            FinishConnectAttempt(false, "Failed to connect remote device");

    case kDisconnected:
        if (current_state_ == kConnected)
            source_->Release();

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

void MiracastService::OnDeviceStateChanged(const NetworkDevice::Ptr &peer) {
    if (peer->State() == kConnected)
        AdvanceState(kConnected);
    else if (peer->State() == kDisconnected) {
        AdvanceState(kDisconnected);
        current_peer_.reset();
    }
    else if (peer->State() == kFailure) {
        AdvanceState(kFailure);
        current_peer_.reset();
        FinishConnectAttempt(false, "Failed to connect device");
    }
}

void MiracastService::OnDeviceFound(const NetworkDevice::Ptr &peer) {
    if (auto sp = delegate_.lock())
        sp->OnDeviceFound(peer);
}

void MiracastService::OnDeviceLost(const NetworkDevice::Ptr &peer) {
    if (auto sp = delegate_.lock())
        sp->OnDeviceLost(peer);
}

gboolean MiracastService::OnIdleTimer(gpointer user_data) {
    auto inst = static_cast<SharedKeepAlive<MiracastService>*>(user_data)->ShouldDie();
    inst->AdvanceState(kIdle);
    return TRUE;
}

void MiracastService::StartIdleTimer() {
    g_timeout_add(kStateIdleTimeout.count(), &MiracastService::OnIdleTimer, new SharedKeepAlive<MiracastService>{shared_from_this()});
}

void MiracastService::FinishConnectAttempt(bool success, const std::string &error_text) {
    if (connect_callback_)
        connect_callback_(success, error_text);

    connect_callback_ = nullptr;
}

void MiracastService::ConnectSink(const MacAddress &address, std::function<void(bool,std::string)> callback) {
    if (current_peer_.get()) {
        callback(false, "Already connected");
        return;
    }

    NetworkDevice::Ptr device;

    for (auto peer : network_manager_->Devices()) {
        if (peer->Address() != address)
            continue;

        device = peer;
        break;
    }

    if (!device) {
        callback(false, "Couldn't find device");
        return;
    }

    if (!network_manager_->Connect(device)) {
        callback(false, "Failed to connect with remote device");
        return;
    }

    current_peer_ = device;
    connect_callback_ = callback;
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
        callback(kErrorAlreadyInProgress);
        return;
    }

    scan_timeout_source_ = g_timeout_add_seconds(timeout.count(), &MiracastService::OnScanTimeout, this);
    current_scan_callback_ = callback;

    network_manager_->Scan();
}
} // namespace miracast
