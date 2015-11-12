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
#include <QDBusVariant>

#include "miracastserviceadaptor.h"
#include "miracastservice.h"
#include "networkp2pmanager.h"
#include "networkp2pdevice.h"
#include "wfddeviceinfo.h"

MiracastServiceAdaptor::MiracastServiceAdaptor(MiracastService *service) :
    QDBusAbstractAdaptor(service),
    service(service),
    scheduledPeersUpdate(false),
    currentConnectId(-1)
{
    connect(service, SIGNAL(stateChanged()),
            this, SLOT(onServiceStateChanged()));
}

void MiracastServiceAdaptor::handlePeerFound(const NetworkP2pDevice::Ptr &peer)
{
    peersAdded.append(peer);
    schedulePeersChanged();
}

void MiracastServiceAdaptor::handlePeerChanged(const NetworkP2pDevice::Ptr &peer)
{
    peersAdded.append(peer);
    schedulePeersChanged();
}

void MiracastServiceAdaptor::handlePeerLost(const NetworkP2pDevice::Ptr &peer)
{
    peersRemoved.append(peer);
    schedulePeersChanged();
}

void MiracastServiceAdaptor::onServiceStateChanged()
{
    sendPropertyChanged("State", NetworkP2pDevice::stateToStr(service->state()));
}

void MiracastServiceAdaptor::sendPropertyChanged(const QString &name, const QVariant &value)
{
    auto signal = QDBusMessage::createSignal("/", "org.freedesktop.DBus.Properties", "PropertiesChanged");
    signal << QString("com.ubuntu.miracast.Manager");

    QVariantMap updatedProperties;
    updatedProperties.insert(name, value);
    signal << updatedProperties;

    signal << QStringList();

    QDBusConnection::systemBus().send(signal);
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
        QVariantMap added;

        for (auto peer : peersAdded) {
            QVariantMap info;
            appendPeer(info, peer);
            added.insert(peer->address(), QVariant(info));
        }

        QStringList removed;
        for (auto peer : peersRemoved)
            removed.append(peer->address());

        QDBusMessage signal = QDBusMessage::createSignal("/",
                                  "com.ubuntu.miracast.Manager", "PeersChanged");
        signal << added;
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

void MiracastServiceAdaptor::replyWithError(const QDBusMessage &message, QDBusError::ErrorType type, const QString &text)
{
    auto reply = message.createErrorReply(type, text);
    QDBusConnection::systemBus().send(reply);
}

int MiracastServiceAdaptor::storeDelayedReply(const QDBusMessage &message)
{
    static int nextId = 0;

    message.setDelayedReply(true);

    auto id = nextId++;
    delayedMessages.insert(id, message);
    return id;
}

void MiracastServiceAdaptor::ConnectSink(const QString &address, const QDBusMessage &message)
{
    if (address.size() == 0) {
        replyWithError(message, QDBusError::InvalidArgs);
        return;
    }

    if (currentConnectId >= 0) {
        replyWithError(message, QDBusError::Failed, "Already connecting");
    }

    // We need to save a class reference here as otherwise it got missed
    // and not carried into the lambda below.
    currentConnectId = storeDelayedReply(message);

    service->connectSink(address, [&](bool success, const QString &errorText) {
        if (currentConnectId < 0)
            return;

        if (!delayedMessages.contains(currentConnectId))
            return;

        QDBusMessage msg = delayedMessages.value(currentConnectId);

        if (!success) {
            replyWithError(msg, QDBusError::Failed, errorText);
            currentConnectId = -1;
            return;
        }

        QDBusConnection::systemBus().send(msg.createReply());

        delayedMessages.remove(currentConnectId);
        currentConnectId = -1;
    });
}

void MiracastServiceAdaptor::DisconnectAll(const QDBusMessage &message)
{
    if (service->state() != NetworkP2pDevice::Association &&
            service->state() != NetworkP2pDevice::Connected) {

        replyWithError(message, QDBusError::Failed, "Not connected");
        return;
    }
    service->networkManager()->disconnectAll();
}
