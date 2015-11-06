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

#ifndef NETWORKP2PMANAGERWPASUPPLICANT_H_
#define NETWORKP2PMANAGERWPASUPPLICANT_H_

#include <functional>

#include <QObject>
#include <QStringList>
#include <QProcess>
#include <QSocketNotifier>

#include "dhcpclient.h"
#include "networkp2pmanager.h"

class NetworkP2pManagerWpaSupplicant : public NetworkP2pManager
{
    Q_OBJECT

public:
    NetworkP2pManagerWpaSupplicant(const QString &iface);
    ~NetworkP2pManagerWpaSupplicant();

    void setWfdSubElements(const QStringList &elements);

    void scan(unsigned int timeout = 30);
    QStringList getPeers();

    int connect(const QString &address, bool persistent = true);
    int disconnectAll();

private Q_SLOTS:
    void onSupplicantFinished(int errorCode);
    void onSupplicantError(QProcess::ProcessError error);
    void onSocketReadyRead();

private:
    void startSupplicant();
    void connectToSupplicant();

    int request(const QString &command, std::function<void(QString)> result);
    void handleUnsolicitedMessage(const QString &message);

    int bytesPendingToRead();

    bool checkResult(const QString &result);

private:
    QString interface;
    QProcess *supplicantProcess;
    QString ctrlPath;
    int sock;
    QSocketNotifier *notifier;
    QStringList peers;
    DhcpClient dhcp;
};

#endif
