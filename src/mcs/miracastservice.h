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

#include <functional>

#include <glib.h>

#include "miracastsource.h"
#include "networkmanager.h"
#include "networkdevice.h"

class MiracastService : public NetworkManager::Delegate,
                        public MiracastSource::Delegate
{
public:
    class Delegate {
    public:
        virtual void OnStateChanged(NetworkDeviceState state) { }
        virtual void OnDeviceFound(const NetworkDevice::Ptr &peer) { }
        virtual void OnDeviceLost(const NetworkDevice::Ptr &peer) { }
    };

    MiracastService();
    ~MiracastService();

    void SetDelegate(Delegate *delegate);

    void ConnectSink(const std::string &address, std::function<void(bool,std::string)> callback);
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
    void AdvanceState(NetworkDeviceState new_state);
    void FinishConnectAttempt(bool success, const std::string &error_text = "");
    void StartIdleTimer();
    void LoadWiFiFirmware();

private:
    Delegate *delegate_;
    NetworkManager *manager_;
    MiracastSource source_;
    NetworkDeviceState current_state_;
    NetworkDevice::Ptr current_peer_;
    std::function<void(bool,std::string)> connect_callback_;
};

#endif
