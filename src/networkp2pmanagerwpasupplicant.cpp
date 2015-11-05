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

#include <sys/select.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <QDebug>
#include <QDataStream>
#include <QTimer>

#include "networkp2pmanagerwpasupplicant.h"
#include "wfddeviceinfo.h"

/*
 * Supplicant message names
 */

#define P2P_DEVICE_FOUND  "P2P-DEVICE-FOUND"
#define P2P_DEVICE_LOST   "P2P-DEVICE-LOST"

NetworkP2pManagerWpaSupplicant::NetworkP2pManagerWpaSupplicant(const QString &iface) :
    interface(iface),
    supplicantProcess(new QProcess(this)),
    ctrlPath(QString("/var/run/%1_supplicant").arg(interface))
{
    startSupplicant();
}

NetworkP2pManagerWpaSupplicant::~NetworkP2pManagerWpaSupplicant()
{
    if (supplicantProcess->state() == QProcess::Running)
        supplicantProcess->close();
}

void NetworkP2pManagerWpaSupplicant::startSupplicant()
{
    QString path = "/home/phablet/wpa_supplicant";
    QStringList arguments;
    arguments << "-Dnl80211"
              << QString("-i%1").arg(interface)
              << QString("-C%1").arg(ctrlPath)
              << QString("-ddd")
              << QString("-t")
              << QString("-K");

    qDebug() << "arguments: " << arguments;
    qDebug() << "path" << path;

    QObject::connect(supplicantProcess, SIGNAL(error(QProcess::ProcessError)),
                     this, SLOT(onSupplicantError(QProcess::ProcessError)));
    QObject::connect(supplicantProcess, SIGNAL(finished(int)),
                     this, SLOT(onSupplicantFinished(int)));

    supplicantProcess->start(path, arguments);
    supplicantProcess->waitForStarted();

    if (supplicantProcess->state() != QProcess::Running) {
        qWarning() << "Failed to start wpa-supplicant!";
        return;
    }

    QTimer::singleShot(200, [=]() { connectToSupplicant(); });
}

bool NetworkP2pManagerWpaSupplicant::checkResult(const QString &result)
{
    if (result.size() == 0)
        return true;

    return result.size() == 3 && result == "OK\n";
}

void NetworkP2pManagerWpaSupplicant::connectToSupplicant()
{
    qDebug() << "Connecting to wpa-supplicant ...";

    QString socketPath = QString("%1/%2").arg(ctrlPath).arg(interface);

    struct sockaddr_un local;
    sock = ::socket(PF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        qWarning() << "Failed to create socket";
        return;
    }

    local.sun_family = AF_UNIX;

    QString localPath = QString("/tmp/p2p0-%1").arg(getpid());
    strncpy(local.sun_path, localPath.toUtf8().constData(), sizeof(local.sun_path));

    if (::bind(sock, (struct sockaddr *) &local, sizeof(local)) < 0) {
        qWarning() << "Failed to bind socket";
        return;
    }

    struct sockaddr_un dest;
    dest.sun_family = AF_UNIX;
    strncpy(dest.sun_path, socketPath.toUtf8().constData(), sizeof(dest.sun_path));

    qDebug() << "sun_path:" << dest.sun_path;
    qDebug() << "socketPath:" << socketPath;

    if (::connect(sock, (struct sockaddr*) &dest, sizeof(dest)) < 0) {
        qWarning() << "Failed to connect socket";
        return;
    }

    int flags = ::fcntl(sock, F_GETFL);
    flags |= O_NONBLOCK;
    ::fcntl(sock, F_SETFL, flags);

    notifier = new QSocketNotifier(sock, QSocketNotifier::Read, this);
    QObject::connect(notifier, SIGNAL(activated(int)), SLOT(onSocketReadyRead()));

    // We need to attach to receive all occuring events from wpa-supplicant
    request("ATTACH", [=](const QString &result) {
        if (!checkResult(result))
            return;
    });

    // Enable WiFi display support
    request("SET wifi_display 1", [=](const QString &result) { });

    QStringList wfdSubElements;
    // FIXME build this rather than specifying a static string here
    wfdSubElements << "000600101C440032";
    setWfdSubElements(wfdSubElements);
}

void NetworkP2pManagerWpaSupplicant::onSupplicantError(QProcess::ProcessError error)
{
    qDebug() << "Supplicant error: " << error;
}

void NetworkP2pManagerWpaSupplicant::onSupplicantFinished(int errorCode)
{
    qDebug() << "Supplicant finished: " << errorCode;
}

int NetworkP2pManagerWpaSupplicant::bytesPendingToRead()
{
    struct timeval tv;
    fd_set rfds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    select(sock + 1, &rfds, NULL, NULL, &tv);
    return FD_ISSET(sock, &rfds);
}

void NetworkP2pManagerWpaSupplicant::onSocketReadyRead()
{
    char buf[256];

    while (bytesPendingToRead() > 0) {

        int ret = recv(sock, buf, sizeof(buf) - 1, 0);
        if (ret < 0)
            return;

        buf[ret] = '\0';

        if (ret > 0 && buf[0] == '<')
            handleUnsolicitedMessage(QString(buf));
    }
}

int NetworkP2pManagerWpaSupplicant::request(const QString &command, std::function<void(QString)> result)
{
    qDebug() << "OUT:" << command;

    if (send(sock, command.toUtf8().constData(), command.size(), 0) < 0) {
        qWarning() << "Failed to send data to wpa-supplicant";
        return 0;
    }

    struct timeval tv;
    fd_set rfds;
    int res;
    char buf[1024];

    for (;;) {
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);

        res = select(sock + 1, &rfds, NULL, NULL, &tv);
        if (res < 0)
            return res;

        if (FD_ISSET(sock, &rfds)) {
            res = recv(sock, buf, sizeof(buf) - 1, 0);
            if (res < 0)
                return res;

            buf[res] = '\0';

            if (res > 0 && buf[0] == '<') {
                handleUnsolicitedMessage(QString(buf));
                continue;
            }

            result(QString(buf));

            return 0;
        }
    }

    return 0;
}

int parseHex(const QString &str)
{
    auto hexStr = str;
    if (hexStr.startsWith("0x") || hexStr.startsWith("0X"))
        hexStr = hexStr.mid(2);

    bool ok = true;
    auto num = hexStr.toInt(&ok, 16);
    if (!ok)
        return 0;

    return num;
}

void NetworkP2pManagerWpaSupplicant::handleUnsolicitedMessage(const QString &message)
{
    auto realMessage = message.mid(3);

    qDebug() << "Got unsolicited reply:" << realMessage;

    if (realMessage.startsWith(P2P_DEVICE_FOUND)) {
        // P2P-DEVICE-FOUND 4e:74:03:70:e2:c1 p2p_dev_addr=4e:74:03:70:e2:c1
        // pri_dev_type=8-0050F204-2 name='Aquaris M10' config_methods=0x188 dev_capab=0x5
        // group_capab=0x0 wfd_dev_info=0x00111c440032 new=1
        QStringList items = realMessage.split(QRegExp(" (?=[^']*('[^']*'[^']*)*$)"));

        auto address = items[1];
        auto wfdDevInfoStr = items[8].section('=', 1).mid(2);
        WfdDevInfo wfdDevInfo(parseHex(wfdDevInfoStr.left(4)),
                              parseHex(wfdDevInfoStr.mid(4, 4)),
                              parseHex(wfdDevInfoStr.right(4)));
        auto configMethods = parseHex(items[5].section('=', 1));

        qDebug() << "address" << address
                 << "deviceType" << wfdDevInfo.deviceTypeAsString()
                 << "controlPort" << wfdDevInfo.controlPort()
                 << "maxThroughput" << wfdDevInfo.maxThroughput()
                 << "configMethods" << configMethods;

        // Only add peer if it is a sink. We don't consider sources at the moment
        auto deviceType = wfdDevInfo.deviceType();
        if (deviceType != WFD_DEVICE_TYPE_PRIMARY_SINK &&
            deviceType != WFD_DEVICE_TYPE_SOURCE_OR_PRIMARY_SINK)
            return;

        // We require WPS PBC for now
        if (!(configMethods & WPS_CONFIG_PUSHBUTTON))
            return;

        peers.push_back(address);
    }
    else if (realMessage.startsWith(P2P_DEVICE_LOST)) {
        // P2P-DEVICE-LOST p2p_dev_addr=4e:74:03:70:e2:c1
        QStringList items = realMessage.split(" ");

        auto addr = items[2].mid(13);
        peers.removeAll(addr);
    }
}

void NetworkP2pManagerWpaSupplicant::setWfdSubElements(const QStringList &elements)
{
    int n = 0;

    for (auto element : elements) {
        auto cmd = QString("WFD_SUBELEM_SET %1 %2").arg(n).arg(element);
        request(cmd, [=](const QString &result) {
            if (!checkResult(result))
                qDebug() << "Failed to set WFD subelement" << n << "with value" << element;
        });

        n++;
    }
}

void NetworkP2pManagerWpaSupplicant::scan(unsigned int timeout)
{
    auto cmd = QString("P2P_FIND %1").arg(timeout);

    request(cmd, [=](const QString &response) {
        qDebug() << "Successfully started scanning for available peers";
    });
}

QStringList NetworkP2pManagerWpaSupplicant::getPeers()
{
    return peers;
}

int NetworkP2pManagerWpaSupplicant::connect(const QString &address)
{
    int ret = 0;

    auto cmd = QString("P2P_CONNECT %1 pbc").arg(address);
    request(cmd, [&](const QString &result) {
        if (!checkResult(result)) {
            ret = -EIO;
            qWarning() << "Failed to connect with remote " << address << ":" << result;
            return;
        }

    });

    return ret;
}

int NetworkP2pManagerWpaSupplicant::disconnectAll()
{
    return 0;
}
