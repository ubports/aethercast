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

#include <glib.h>

#include "miracastsourcemanager.h"
#include "networkmanager.h"
#include "networkdevice.h"

namespace mcs {
class MiracastService : public std::enable_shared_from_this<MiracastService>,
                        public NetworkManager::Delegate,
                        public MiracastSourceManager::Delegate
{
public:
    class Delegate : private boost::noncopyable {
    public:
        virtual void OnStateChanged(NetworkDeviceState state) = 0;
        virtual void OnDeviceFound(const NetworkDevice::Ptr &peer) = 0;
        virtual void OnDeviceLost(const NetworkDevice::Ptr &peer) = 0;

    protected:
        Delegate() = default;
    };

    static std::shared_ptr<MiracastService> create();

    ~MiracastService();

    void SetDelegate(const std::weak_ptr<Delegate> &delegate);
    void ResetDelegate();

    void ConnectSink(const MacAddress &address, std::function<void(bool,std::string)> callback);
    void Scan();

    NetworkDeviceState State() const;

    void OnClientDisconnected();

public:
    void OnDeviceStateChanged(const NetworkDevice::Ptr &peer) override;
    void OnDeviceFound(const NetworkDevice::Ptr &peer) override;
    void OnDeviceLost(const NetworkDevice::Ptr &peer) override;

private:
    static gboolean OnIdleTimer(gpointer user_data);
    static gboolean OnRetryLoadFirmware(gpointer user_data);
    static void OnWiFiFirmwareLoaded(GDBusConnection *conn, GAsyncResult *res, gpointer user_data);

private:
    MiracastService();
    std::shared_ptr<MiracastService> FinalizeConstruction();

    void AdvanceState(NetworkDeviceState new_state);
    void FinishConnectAttempt(bool success, const std::string &error_text = "");
    void StartIdleTimer();
    void LoadWiFiFirmware();

private:
    std::weak_ptr<Delegate> delegate_;
    NetworkManager *manager_;
    std::shared_ptr<MiracastSourceManager> source_;
    NetworkDeviceState current_state_;
    NetworkDevice::Ptr current_peer_;
    std::function<void(bool,std::string)> connect_callback_;
};
} // namespace mcs
#endif
