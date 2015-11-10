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
#include <QObject>

#include "miracastsource.h"
#include "networkp2pdevice.h"
#include "dhcpclient.h"
#include "dhcpserver.h"

class NetworkP2pManager;

class MiracastService : public QObject
{
    Q_OBJECT
public:
    MiracastService();
    ~MiracastService();

    NetworkP2pManager* networkManager() { return manager; }

    void connectSink(const QString &address, std::function<void(bool,QString)> callback);

    NetworkP2pDevice::State state() const;

Q_SIGNALS:
    void stateChanged();

private Q_SLOTS:
    void onPeerChanged(const NetworkP2pDevice::Ptr &peer);
    void onPeerConnected(const NetworkP2pDevice::Ptr &peer);
    void onPeerDisconnected(const NetworkP2pDevice::Ptr &peer);
    void onPeerFailed(const NetworkP2pDevice::Ptr &peer);
    void onLocalClientAddressAssigned(const QString &address);
    void onSourceClientDisconnected();

private:
    void loadRequiredFirmware();
    void advanceState(NetworkP2pDevice::State newState);
    void finishConnectAttempt(bool success, const QString &errorText = "");
    void startIdleTimer();
    void setupDhcp();
    void releaseDhcp();

private:
    NetworkP2pManager *manager;
    MiracastSource source;
    NetworkP2pDevice::State currentState;
    NetworkP2pDevice::Ptr currentPeer;
    std::function<void(bool,QString)> connectCallback;
    DhcpClient dhcpClient;
    DhcpServer dhcpServer;
};

#endif
