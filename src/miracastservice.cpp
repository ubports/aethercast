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

#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusObjectPath>
#include <QTimer>

#include "miracastservice.h"
#include "networkp2pmanagerwpasupplicant.h"

#define MIRACAST_DEFAULT_RTSP_CTRL_PORT     7236

MiracastService::MiracastService()
{
    qDebug() << "Switching device WiFi chip firmware to get P2P support";

    QDBusInterface iface("fi.w1.wpa_supplicant1", "/fi/w1/wpa_supplicant1",
                         "fi.w1.wpa_supplicant1", QDBusConnection::systemBus());

    if (!iface.isValid()) {
        qWarning() << "Could not reach wpa-supplicant on dbus";
        return;
    }

    QDBusReply<void> reply = iface.call("SetInterfaceFirmware",
                                QVariant::fromValue(QDBusObjectPath("/fi/w1/wpa_supplicant1/Interfaces/1")),
                                "p2p");
    if (!reply.isValid()) {
        qDebug() << "Failed to switch WiFi chip firmware:" << reply.error();
        return;
    }

    QTimer::singleShot(200, [=]() {
        manager = new NetworkP2pManagerWpaSupplicant("p2p0");

        connect(manager, SIGNAL(sinkConnected(QString,QString)),
                this, SLOT(onSinkConnected(QString,QString)));
    });
}

MiracastService::~MiracastService()
{
}

void MiracastService::onSinkConnected(const QString &localAddress, const QString &remoteAddress)
{
    qDebug() << "Mircast sink connected remote:" << remoteAddress << "local:" << localAddress;

    source.setup(localAddress, MIRACAST_DEFAULT_RTSP_CTRL_PORT);
}
