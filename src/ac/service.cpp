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
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <chrono>

#include <boost/filesystem.hpp>

#include <wds/logging.h>

#include "ac/config.h"
#include "ac/keep_alive.h"
#include "ac/logger.h"
#include "ac/service.h"
#include "ac/networkmanagerfactory.h"
#include "ac/types.h"
#include "ac/logger.h"

#include "ac/dbus/controllerskeleton.h"

namespace {
// TODO(morphis, tvoss): Expose the port as a construction-time parameter.
const std::uint16_t kMiracastDefaultRtspCtrlPort{7236};
const std::chrono::milliseconds kStateIdleTimeout{5000};
const std::chrono::seconds kShutdownGracePreriod{1};
const std::int16_t kProcessPriorityUrgentDisplay{-8};

// SafeLog serves as integration point to the wds::LogSystem world.
template <ac::Logger::Severity severity>
void SafeLog (const char *format, ...)
{
    static constexpr const std::size_t kBufferSize{256};

    char buffer[kBufferSize];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, kBufferSize, format, args);
    ac::Log().Log(severity, std::string{buffer}, boost::optional<ac::Logger::Location>{});
    va_end (args);
}
}
namespace ac {
Service::MainOptions Service::MainOptions::FromCommandLine(int argc, char** argv) {
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

int Service::Main(const Service::MainOptions &options) {
    if (options.print_version) {
        std::printf("%d.%d\n", Service::kVersionMajor, Service::kVersionMinor);
        return 0;
    }

    if (options.debug)
        ac::Log().Init(ac::Logger::Severity::kDebug);

    struct Runtime {
        static gboolean OnSignalRaised(gpointer user_data) {
            auto thiz = static_cast<Runtime*>(user_data);

            // This will bring down everything and the timeout below will give
            // things a small amount of time to perform their shutdown jobs.
            thiz->service->Shutdown();

            AC_DEBUG("Exiting");

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
            wds::LogSystem::set_vlog_func(SafeLog<ac::Logger::Severity::kTrace>);
            wds::LogSystem::set_log_func(SafeLog<ac::Logger::Severity::kInfo>);
            wds::LogSystem::set_warning_func(SafeLog<ac::Logger::Severity::kWarning>);
            wds::LogSystem::set_error_func(SafeLog<ac::Logger::Severity::kError>);

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

            // Raise our process priority to be as fast as possible
            setpriority(PRIO_PROCESS, 0, kProcessPriorityUrgentDisplay);

            service = ac::Service::Create();
            controller_skeleton = ac::dbus::ControllerSkeleton::Create(service);
        }

        ~Runtime() {
            g_main_loop_unref(ml);
        }

        void Run() {
            g_main_loop_run(ml);
        }

        GMainLoop *ml = g_main_loop_new(nullptr, FALSE);
        ac::Service::Ptr service;
        ac::dbus::ControllerSkeleton::Ptr controller_skeleton;
    } rt;

    rt.Run();

    return 0;
}

std::shared_ptr<Service> Service::Create() {
    auto sp = std::shared_ptr<Service>{new Service{}};
    return sp->FinalizeConstruction();
}

Service::Service() :
    current_state_(kIdle),
    scan_timeout_source_(0),
    supported_roles_({kSource}),
    enabled_(false) {

    CreateRuntimeDirectory();
}

std::shared_ptr<Service> Service::FinalizeConstruction() {
    system_controller_ = ac::SystemController::CreatePlatformDefault();

    network_manager_ = ac::NetworkManagerFactory::Create();
    network_manager_->SetDelegate(this);
    network_manager_->SetCapabilities({NetworkManager::Capability::kSource});

    LoadState();

    return shared_from_this();
}

Service::~Service() {
    if (scan_timeout_source_ > 0)
        g_source_remove(scan_timeout_source_);
}

void Service::CreateRuntimeDirectory() {
    boost::filesystem::path runtime_dir(ac::kRuntimePath);

    if (boost::filesystem::is_directory(runtime_dir))
        boost::filesystem::remove_all(runtime_dir);

    boost::filesystem::create_directory(runtime_dir);
}

void Service::LoadState() {
    boost::filesystem::path state_dir(ac::kStateDir);

    if (!boost::filesystem::is_directory(state_dir))
        return;

    auto enabled_path = state_dir / "enabled";
    SetEnabled(boost::filesystem::exists(enabled_path));
}

void Service::SaveState() {
    boost::filesystem::path state_dir(ac::kStateDir);

    if (!boost::filesystem::is_directory(state_dir))
        boost::filesystem::create_directory(state_dir);

    auto enabled_path = state_dir / "enabled";
    if (enabled_)
        ac::Utils::CreateFile(enabled_path.string());
    else
        ac::Utils::RemoveFile(enabled_path.string());
}

void Service::SetDelegate(const std::weak_ptr<Controller::Delegate> &delegate) {
    delegate_ = delegate;
}

void Service::ResetDelegate() {
    delegate_.reset();
}

NetworkDeviceState Service::State() const {
    return current_state_;
}

std::vector<NetworkManager::Capability> Service::Capabilities() const {
    if (!enabled_)
        return std::vector<NetworkManager::Capability>{};

    return network_manager_->Capabilities();
}

bool Service::Scanning() const {
    if (!enabled_)
        return false;

    return network_manager_->Scanning();
}

bool Service::Enabled() const {
    return network_manager_->Ready() && enabled_;
}

bool Service::SetupNetworkManager() {
    return network_manager_->Setup();
}

bool Service::ReleaseNetworkManager() {
    if (current_device_)
        network_manager_->Disconnect(current_device_);

    network_manager_->Release();

    current_device_.reset();

    return true;
}

Error Service::SetEnabled(bool enabled) {
    if (!network_manager_->Ready())
        return Error::kNotReady;

    return SetEnabledInternal(enabled, false);
}

Error Service::SetEnabledInternal(bool enabled, bool no_save) {
    if (enabled_ == enabled)
        return Error::kNone;

    if (enabled && !SetupNetworkManager())
        return Error::kFailed;
    else if (!enabled && !ReleaseNetworkManager())
        return Error::kFailed;

    enabled_ = enabled;

    if (!no_save)
        SaveState();

    if (auto sp = delegate_.lock())
        sp->OnChanged();

    return Error::kNone;
}

void Service::OnClientDisconnected() {
    g_timeout_add(0, [](gpointer user_data) {
        auto thiz = static_cast<WeakKeepAlive<Service>*>(user_data)->GetInstance().lock();

        if (!thiz)
            return FALSE;

        thiz->Disconnect(thiz->current_device_, nullptr);

        return FALSE;
    }, new WeakKeepAlive<Service>(shared_from_this()));
}

void Service::AdvanceState(NetworkDeviceState new_state) {
    DEBUG("new state %s current state %s",
          ac::NetworkDevice::StateToStr(new_state),
          ac::NetworkDevice::StateToStr(current_state_));

    switch (new_state) {
    case kAssociation:
        break;

    case kConfiguration:
        break;

    case kConnected:
        source_ = SourceManager::Create(network_manager_->LocalAddress(), kMiracastDefaultRtspCtrlPort);
        source_->SetDelegate(shared_from_this());
        FinishConnectAttempt();
        break;

    case kFailure:
        FinishConnectAttempt(Error::kFailed);

    case kDisconnected:
        source_.reset();
        current_device_.reset();

        system_controller_->DisplayStateLock()->Release(ac::DisplayState::On);

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

void Service::OnChanged() {
   if (auto sp = delegate_.lock())
       sp->OnChanged();
}

void Service::OnDeviceStateChanged(const NetworkDevice::Ptr &device) {
    DEBUG("Device state changed: address %s new state %s",
          device->Address(),
          ac::NetworkDevice::StateToStr(device->State()));

    if (device != current_device_)
        return;

    AdvanceState(device->State());

    if (auto sp = delegate_.lock())
        sp->OnDeviceChanged(device);
}

void Service::OnDeviceChanged(const NetworkDevice::Ptr &device) {
    if (auto sp = delegate_.lock())
        sp->OnDeviceChanged(device);
}

void Service::OnDeviceFound(const NetworkDevice::Ptr &device) {
    if (auto sp = delegate_.lock())
        sp->OnDeviceFound(device);
}

void Service::OnDeviceLost(const NetworkDevice::Ptr &device) {
    if (auto sp = delegate_.lock())
        sp->OnDeviceLost(device);
}

void Service::OnReadyChanged() {
    if (!network_manager_->Ready())
        // NetworkManager switch on its own to not ready so we avoid
        // storing the new 'Enabled' state as that
        SetEnabledInternal(false, true);
    else
        LoadState();
}

gboolean Service::OnIdleTimer(gpointer user_data) {
    auto inst = static_cast<SharedKeepAlive<Service>*>(user_data)->ShouldDie();
    inst->AdvanceState(kIdle);
    return FALSE;
}

void Service::StartIdleTimer() {
    g_timeout_add(kStateIdleTimeout.count(), &Service::OnIdleTimer,
                  new SharedKeepAlive<Service>{shared_from_this()});
}

void Service::FinishConnectAttempt(ac::Error error) {
    if (connect_callback_)
        connect_callback_(error);

    connect_callback_ = nullptr;
}

void Service::Connect(const NetworkDevice::Ptr &device, ResultCallback callback) {
    if (!enabled_) {
        AC_DEBUG("Not ready");
        callback(Error::kNotReady);
        return;
    }

    if (current_device_) {
        AC_DEBUG("Tried to connect again while we're already trying to connect a device");
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

    system_controller_->DisplayStateLock()->Acquire(ac::DisplayState::On);

    current_device_ = device;
    connect_callback_ = callback;
}

void Service::Disconnect(const NetworkDevice::Ptr &device, ResultCallback callback) {
    if (!enabled_) {
        callback(Error::kNotReady);
        return;
    }

    if (!current_device_ || !device) {
        if (callback)
            callback(Error::kParamInvalid);
        return;
    }

    if (!network_manager_->Disconnect(device)) {
        if (callback)
            callback(Error::kFailed);
        return;
    }

    if (callback)
        callback(Error::kNone);
}

void Service::DisconnectAll(ResultCallback callback) {
    Disconnect(current_device_, callback);
}

ac::Error Service::Scan(const std::chrono::seconds &timeout) {
    if (!enabled_)
        return Error::kNotReady;

    if (current_device_)
        return ac::Error::kNotReady;

    network_manager_->Scan(timeout);

    return ac::Error::kNone;
}

void Service::Shutdown() {
    SetEnabledInternal(false, true);
}

} // namespace ac
