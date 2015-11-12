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
#include "networkp2pmanager.h"
#include "networkp2pdevice.h"
#include "dhcpclient.h"
#include "dhcpserver.h"

class MiracastService : public QObject,
                        public NetworkP2pManager::Delegate
{
    Q_OBJECT
public:
    MiracastService();
    ~MiracastService();

    NetworkP2pManager* networkManager() { return manager; }

    void connectSink(const QString &address, std::function<void(bool,QString)> callback);

    NetworkP2pDevice::State state() const;

public:
    void peerChanged(const NetworkP2pDevice::Ptr &peer);
    void peerConnected(const NetworkP2pDevice::Ptr &peer);
    void peerDisconnected(const NetworkP2pDevice::Ptr &peer);
    void peerFailed(const NetworkP2pDevice::Ptr &peer);

private Q_SLOTS:
    void onSourceClientDisconnected();

Q_SIGNALS:
    void stateChanged();

private:
    void loadRequiredFirmware();
    void advanceState(NetworkP2pDevice::State newState);
    void finishConnectAttempt(bool success, const QString &errorText = "");
    void startIdleTimer();

private:
    NetworkP2pManager *manager;
    MiracastSource source;
    NetworkP2pDevice::State currentState;
    NetworkP2pDevice::Ptr currentPeer;
    std::function<void(bool,QString)> connectCallback;
};

#endif
