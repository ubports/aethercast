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

#ifndef MIRACAST_SERVICE_H_
#define MIRACAST_SERVICE_H_

#include <boost/noncopyable.hpp>

#include <functional>
#include <memory>
#include <chrono>

#include <glib.h>

#include "miracastsourcemanager.h"
#include "networkmanager.h"
#include "networkdevice.h"
#include "non_copyable.h"
#include "types.h"

namespace mcs {
class MiracastService : public std::enable_shared_from_this<MiracastService>,
                        public NetworkManager::Delegate,
                        public MiracastSourceManager::Delegate
{
public:
    static constexpr const uint kVersionMajor = 0;
    static constexpr const uint kVersionMinor = 1;

    typedef std::shared_ptr<MiracastService> Ptr;

    struct MainOptions {
        static MainOptions FromCommandLine(int argc, char** argv);

        bool debug;
        bool print_version;
    };

    static int Main(const MainOptions &options);

    class Delegate : private mcs::NonCopyable {
    public:
        virtual ~Delegate() { }

        virtual void OnStateChanged(NetworkDeviceState state) = 0;
        virtual void OnDeviceFound(const NetworkDevice::Ptr &device) = 0;
        virtual void OnDeviceLost(const NetworkDevice::Ptr &device) = 0;
        virtual void OnDeviceChanged(const NetworkDevice::Ptr &device) = 0;

    protected:
        Delegate() = default;
    };

    static std::shared_ptr<MiracastService> Create();

    ~MiracastService();

    void SetDelegate(const std::weak_ptr<Delegate> &delegate);
    void ResetDelegate();

    void Connect(const NetworkDevice::Ptr &device, ResultCallback callback);
    void Disconnect(const NetworkDevice::Ptr &device, ResultCallback callback);

    void Scan(ResultCallback callback, const std::chrono::seconds &timeout = std::chrono::seconds{30});

    NetworkDeviceState State() const;

    void OnClientDisconnected();

public:
    void OnDeviceStateChanged(const NetworkDevice::Ptr &device) override;
    void OnDeviceChanged(const NetworkDevice::Ptr &device) override;
    void OnDeviceFound(const NetworkDevice::Ptr &device) override;
    void OnDeviceLost(const NetworkDevice::Ptr &device) override;

private:
    static gboolean OnIdleTimer(gpointer user_data);
    static gboolean OnScanTimeout(gpointer user_data);

private:
    MiracastService();
    std::shared_ptr<MiracastService> FinalizeConstruction();

    void AdvanceState(NetworkDeviceState new_state);
    void FinishConnectAttempt(mcs::Error error = kErrorNone);
    void StartIdleTimer();
    void LoadWiFiFirmware();

private:
    std::weak_ptr<Delegate> delegate_;
    std::shared_ptr<NetworkManager> network_manager_;
    std::shared_ptr<MiracastSourceManager> source_;
    NetworkDeviceState current_state_;
    NetworkDevice::Ptr current_device_;
    ResultCallback connect_callback_;
    guint scan_timeout_source_;
    ResultCallback current_scan_callback_;
};
} // namespace mcs
#endif
