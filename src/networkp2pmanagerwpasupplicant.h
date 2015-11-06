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
#include <QPointer>

#include "dhcpclient.h"
#include "networkp2pmanager.h"

class WpaSupplicantParser;
class WpaSupplicantCommandQueue;

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

    State state() const override;

Q_SIGNALS:
    void sinkConnected(const QString &localAddress, const QString &remoteAddress);

private Q_SLOTS:
    void onSupplicantFinished(int errorCode);
    void onSupplicantError(QProcess::ProcessError error);
    void onSocketReadyRead();
    void onTransportWriteNeeded(const QString &message);
    void onUnsolicitedResponse(const QString &response);

private:
    void startSupplicant();
    void connectToSupplicant();

    void request(const QString &command, std::function<void(QString)> result);

    bool checkResult(const QString &result);

    int bytesAvailableToRead();

private:
    QString interface;
    QProcess *supplicantProcess;
    QString ctrlPath;
    int sock;
    QSocketNotifier *notifier;
    QStringList peers;
    DhcpClient dhcp;
    QPointer<WpaSupplicantParser> parser;
    QPointer<WpaSupplicantCommandQueue> commandQueue;
};

#endif
