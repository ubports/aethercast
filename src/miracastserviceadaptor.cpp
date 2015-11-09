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

#include <QTimer>
#include <QDBusMessage>
#include <QDBusConnection>

#include "miracastserviceadaptor.h"
#include "miracastservice.h"
#include "networkp2pmanager.h"
#include "wfddeviceinfo.h"

MiracastServiceAdaptor::MiracastServiceAdaptor(MiracastService *service) :
    QDBusAbstractAdaptor(service),
    service(service),
    scheduledPeersUpdate(false)
{
    QObject::connect(service->networkManager(), &NetworkP2pManager::peerFound,
            [&](const NetworkP2pDevice::Ptr &peer) {
        peersAdded.append(peer);
        schedulePeersChanged();
    });

    connect(service->networkManager(), &NetworkP2pManager::peerLost,
            [&](const NetworkP2pDevice::Ptr &peer) {
        peersRemoved.append(peer);
        schedulePeersChanged();
    });
}

void MiracastServiceAdaptor::appendPeer(QVariantMap &info, const NetworkP2pDevice::Ptr &peer)
{
    info.insert("Name", peer->name());
    info.insert("State", peer->stateAsString());
    info.insert("Address", peer->address());

    QVariantMap ipv4Info;
    info.insert("IPv4", ipv4Info);

    QVariantMap wfdDeviceInfo;
    auto wfdInfo = peer->wfdDeviceInfo();
    wfdDeviceInfo.insert("Type", wfdInfo.deviceTypeAsString());
    wfdDeviceInfo.insert("ControlPort", wfdInfo.controlPort());
    wfdDeviceInfo.insert("MaxThroughput", wfdInfo.maxThroughput());
    info.insert("WfdDeviceInfo", wfdDeviceInfo);
}

void MiracastServiceAdaptor::schedulePeersChanged()
{
    if (scheduledPeersUpdate)
        return;

    scheduledPeersUpdate = true;

    QTimer::singleShot(100, [=]() {
        QList<QVariant> added;

        for (auto peer : peersAdded) {
            QVariantMap info;
            appendPeer(info, peer);
            added.append(QVariant(info));
        }

        QStringList removed;
        for (auto peer : peersRemoved)
            removed.append(peer->address());

        QDBusMessage signal = QDBusMessage::createSignal("/",
                                  "com.ubuntu.miracast.Manager", "PeersChanged");
        signal << QVariant(added);
        signal << removed;

        scheduledPeersUpdate = false;
    });
}

void MiracastServiceAdaptor::Scan()
{
    service->networkManager()->scan();
}

void MiracastServiceAdaptor::GetPeers(const QDBusMessage &message)
{
    QList<QVariant> result;

    auto peers = service->networkManager()->peers();

    for (auto peer : peers) {
        QVariantMap peerInfo;
        appendPeer(peerInfo, peer);
        result.push_back(QVariant(peerInfo));
    }

    auto reply = message.createReply(QVariant(result));
    QDBusConnection::systemBus().send(reply);
}

void MiracastServiceAdaptor::replyWithError(const QDBusMessage &message, const QString &text)
{
    auto reply = message.createError(QDBusError::InvalidArgs, text);
    QDBusConnection::systemBus().send(reply);
}

void MiracastServiceAdaptor::ConnectSink(const QString &address, const QDBusMessage &message)
{
    if (address.size() == 0) {
        replyWithError(message);
        return;
    }

    // FIXME ConnectSink should be async and only return when the connection attempt either failed
    // or succeeded.
    service->connectSink(address);
}

void MiracastServiceAdaptor::DisconnectAll()
{
    service->networkManager()->disconnectAll();
}
