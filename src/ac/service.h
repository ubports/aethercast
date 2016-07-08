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

#ifndef AC_SERVICE_H_
#define AC_SERVICE_H_

#include <boost/noncopyable.hpp>

#include <functional>
#include <memory>
#include <chrono>

#include "ac/glib_wrapper.h"
#include "ac/controller.h"
#include "ac/sourcemanager.h"
#include "ac/networkmanager.h"
#include "ac/networkdevice.h"
#include "ac/non_copyable.h"
#include "ac/types.h"
#include "ac/systemcontroller.h"

namespace ac {
class Service : public Controller,
                public std::enable_shared_from_this<Service>,
                public NetworkManager::Delegate,
                public SourceManager::Delegate
{
public:
    static constexpr const uint kVersionMajor = 0;
    static constexpr const uint kVersionMinor = 1;

    typedef std::shared_ptr<Service> Ptr;

    struct MainOptions {
        static MainOptions FromCommandLine(int argc, char** argv);

        bool debug;
        bool print_version;
    };

    static int Main(const MainOptions &options);

    static std::shared_ptr<Service> Create();

    ~Service();

    void SetDelegate(const std::weak_ptr<Controller::Delegate> &delegate);
    void ResetDelegate();

    void Connect(const NetworkDevice::Ptr &device, ResultCallback callback);
    void Disconnect(const NetworkDevice::Ptr &device, ResultCallback callback);
    void DisconnectAll(ResultCallback callback);

    ac::Error Scan(const std::chrono::seconds &timeout = std::chrono::seconds{30});

    NetworkDeviceState State() const;
    std::vector<NetworkManager::Capability> Capabilities() const;
    bool Scanning() const;
    bool Enabled() const;

    Error SetEnabled(bool enabled) override;

    void OnClientDisconnected();

    bool SetupNetworkManager();
    bool ReleaseNetworkManager();

public:
    void OnDeviceStateChanged(const NetworkDevice::Ptr &device) override;
    void OnDeviceChanged(const NetworkDevice::Ptr &device) override;
    void OnDeviceFound(const NetworkDevice::Ptr &device) override;
    void OnDeviceLost(const NetworkDevice::Ptr &device) override;
    void OnChanged() override;
    void OnReadyChanged() override;

private:
    static gboolean OnIdleTimer(gpointer user_data);

private:
    Service();
    std::shared_ptr<Service> FinalizeConstruction();

    void AdvanceState(NetworkDeviceState new_state);
    void FinishConnectAttempt(ac::Error error = ac::Error::kNone);
    void StartIdleTimer();
    void LoadWiFiFirmware();

    void Shutdown();
    void CreateRuntimeDirectory();

    Error SetEnabledInternal(bool enabled, bool no_save);

    void LoadState();
    void SaveState();

    bool IsConnecting() const;

private:
    std::weak_ptr<Controller::Delegate> delegate_;
    std::shared_ptr<NetworkManager> network_manager_;
    std::shared_ptr<SourceManager> source_;
    NetworkDeviceState current_state_;
    NetworkDevice::Ptr current_device_;
    ResultCallback connect_callback_;
    guint scan_timeout_source_;
    ResultCallback current_scan_callback_;
    std::vector<NetworkDeviceRole> supported_roles_;
    ac::SystemController::Ptr system_controller_;
    bool enabled_;
};
} // namespace ac
#endif
