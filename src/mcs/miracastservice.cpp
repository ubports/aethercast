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

#include <boost/filesystem.hpp>

#include <wds/logging.h>

#include "config.h"
#include "keep_alive.h"
#include "logger.h"
#include "miracastservice.h"
#include "miracastcontrollerskeleton.h"
#include "networkmanagerfactory.h"
#include "types.h"
#include "logging.h"

namespace {
// TODO(morphis, tvoss): Expose the port as a construction-time parameter.
const std::uint16_t kMiracastDefaultRtspCtrlPort{7236};
const std::chrono::milliseconds kStateIdleTimeout{5000};
const std::chrono::seconds kShutdownGracePreriod{1};

// SafeLog serves as integration point to the wds::LogSystem world.
template <mcs::Logger::Severity severity>
void SafeLog (const char *format, ...)
{
    static constexpr const std::size_t kBufferSize{256};

    char buffer[kBufferSize];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, kBufferSize, format, args);
    mcs::Log().Log(severity, std::string{buffer}, boost::optional<mcs::Logger::Location>{});
    va_end (args);
}
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
        static gboolean OnSignalRaised(gpointer user_data) {
            auto thiz = static_cast<Runtime*>(user_data);

            // This will bring down everything and the timeout below will give
            // things a small amount of time to perform their shutdown jobs.
            thiz->service->Shutdown();

            MCS_DEBUG("Exiting");

            g_timeout_add_seconds(kShutdownGracePreriod.count(), [](gpointer user_data) {
                auto thiz = static_cast<Runtime*>(user_data);
                g_main_loop_quit(thiz->ml);
                return FALSE;
            }, thiz);

            // A second SIGTERM should really terminate us and also overlay
            // the grace period for a proper shutdown we're performing.
            return FALSE;
        }

        Runtime() {
            // We do not have to use a KeepAlive<Scope> here as
            // a Runtime instance controls the lifetime of signal
            // emissions.
            g_unix_signal_add(SIGINT, OnSignalRaised, this);
            g_unix_signal_add(SIGTERM, OnSignalRaised, this);

            // Redirect all wds logging to our own.
            wds::LogSystem::set_vlog_func(SafeLog<mcs::Logger::Severity::kTrace>);
            wds::LogSystem::set_log_func(SafeLog<mcs::Logger::Severity::kInfo>);
            wds::LogSystem::set_warning_func(SafeLog<mcs::Logger::Severity::kWarning>);
            wds::LogSystem::set_error_func(SafeLog<mcs::Logger::Severity::kError>);

            // Redirect all g* logging to our own.
            g_log_set_default_handler([](const gchar *domain, GLogLevelFlags log_level, const gchar *msg, gpointer) {
                switch (log_level & G_LOG_LEVEL_MASK) {
                case G_LOG_LEVEL_DEBUG:
                    Log().Debug(msg);
                    break;
                case G_LOG_LEVEL_INFO:
                    Log().Info(msg);
                    break;
                case G_LOG_LEVEL_MESSAGE:
                    Log().Info(msg);
                    break;
                case G_LOG_LEVEL_WARNING:
                    Log().Warning(msg);
                    break;
                case G_LOG_LEVEL_CRITICAL:
                    Log().Error(msg);
                    break;
                case G_LOG_LEVEL_ERROR:
                    Log().Fatal(msg);
                    break;
                }
            }, nullptr);

            // Become a reaper of all our children
            if (prctl(PR_SET_CHILD_SUBREAPER, 1) < 0)
                g_warning("Failed to make us a subreaper of our children");

            network_manager = mcs::NetworkManagerFactory::Create();
            service = mcs::MiracastService::Create(network_manager);
            mcsa = mcs::MiracastControllerSkeleton::create(service);
        }

        ~Runtime() {
            g_main_loop_unref(ml);
        }

        void Run() {
            g_main_loop_run(ml);
        }

        GMainLoop *ml = g_main_loop_new(nullptr, FALSE);
        bool is_gst_initialized = gst_init_check(nullptr, nullptr, nullptr);
        mcs::NetworkManager::Ptr network_manager;
        mcs::MiracastService::Ptr service;
        mcs::MiracastControllerSkeleton::Ptr mcsa;
    } rt;

    rt.Run();

    return 0;
}

std::shared_ptr<MiracastService> MiracastService::Create(const NetworkManager::Ptr &network_manager) {
    auto sp = std::shared_ptr<MiracastService>{new MiracastService{}};
    return sp->FinalizeConstruction(network_manager);
}

MiracastService::MiracastService() :
    current_state_(kIdle),
    scan_timeout_source_(0),
    supported_roles_({kSource}) {

    CreateRuntimeDirectory();
}

std::shared_ptr<MiracastService> MiracastService::FinalizeConstruction(const NetworkManager::Ptr &network_manager) {
    network_manager_ = network_manager;

    if (network_manager_) {
        network_manager_->SetDelegate(this);
        network_manager_->Setup();
    }

    return shared_from_this();
}

MiracastService::~MiracastService() {
    if (scan_timeout_source_ > 0)
        g_source_remove(scan_timeout_source_);
}

void MiracastService::CreateRuntimeDirectory() {
    boost::filesystem::path runtime_dir(mcs::kRuntimePath);

    if (boost::filesystem::is_directory(runtime_dir))
        boost::filesystem::remove_all(runtime_dir);

    boost::filesystem::create_directory(runtime_dir);
}

void MiracastService::SetDelegate(const std::weak_ptr<MiracastController::Delegate> &delegate) {
    delegate_ = delegate;
}

void MiracastService::ResetDelegate() {
    delegate_.reset();
}

NetworkDeviceState MiracastService::State() const {
    return current_state_;
}

std::vector<NetworkManager::Capability> MiracastService::Capabilities() const {
    return network_manager_->Capabilities();
}

bool MiracastService::Scanning() const {
    return network_manager_->Scanning();
}

void MiracastService::OnClientDisconnected() {
    AdvanceState(kFailure);
    source_.reset();
    current_device_.reset();
}

void MiracastService::AdvanceState(NetworkDeviceState new_state) {
    IpV4Address address;

    DEBUG("new state %s current state %s",
          mcs::NetworkDevice::StateToStr(new_state),
          mcs::NetworkDevice::StateToStr(current_state_));

    switch (new_state) {
    case kAssociation:
        break;

    case kConnected:
        address = network_manager_->LocalAddress();

        source_ = MiracastSourceManager::Create(address, kMiracastDefaultRtspCtrlPort);

        FinishConnectAttempt();

        break;

    case kFailure:
        FinishConnectAttempt(Error::kFailed);

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

void MiracastService::OnChanged() {
   if (auto sp = delegate_.lock())
       sp->OnChanged();
}

void MiracastService::OnDeviceStateChanged(const NetworkDevice::Ptr &device) {
    DEBUG("Device state changed: address %s new state %s",
          device->Address(),
          mcs::NetworkDevice::StateToStr(device->State()));

    if (device != current_device_)
        return;

    AdvanceState(device->State());

    if (auto sp = delegate_.lock())
        sp->OnDeviceChanged(device);
}

void MiracastService::OnDeviceChanged(const NetworkDevice::Ptr &device) {
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
    g_timeout_add(kStateIdleTimeout.count(), &MiracastService::OnIdleTimer,
                  new SharedKeepAlive<MiracastService>{shared_from_this()});
}

void MiracastService::FinishConnectAttempt(mcs::Error error) {
    if (connect_callback_)
        connect_callback_(error);

    connect_callback_ = nullptr;
}

void MiracastService::Connect(const NetworkDevice::Ptr &device, ResultCallback callback) {
    if (current_device_) {
        MCS_DEBUG("Tried to connect again while we're already trying to connect a device");
        callback(Error::kAlready);
        return;
    }

    if (!device) {
        callback(Error::kParamInvalid);
        return;
    }

    DEBUG("address %s", device->Address());

    if (!network_manager_->Connect(device)) {
        DEBUG("Failed to connect remote device");
        callback(Error::kFailed);
        return;
    }

    current_device_ = device;
    connect_callback_ = callback;
}

void MiracastService::Disconnect(const NetworkDevice::Ptr &device, ResultCallback callback) {
    if (!current_device_ || !device) {
        callback(Error::kParamInvalid);
        return;
    }

    if (!network_manager_->Disconnect(device)) {
        callback(Error::kFailed);
        return;
    }

    callback(Error::kNone);
}

void MiracastService::Scan(const std::chrono::seconds &timeout) {
    network_manager_->Scan(timeout);
}

void MiracastService::Shutdown() {
    if (current_device_)
        network_manager_->Disconnect(current_device_);

    network_manager_->Release();
}

} // namespace miracast
