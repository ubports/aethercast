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
#include <sys/ioctl.h>
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
#include <QLocalSocket>
#include <QQueue>

#include "networkp2pmanagerwpasupplicant.h"
#include "wfddeviceinfo.h"

#define READ_BUFFER_SIZE    1024

/*
 * Supplicant message names
 */

#define P2P_DEVICE_FOUND                         "P2P-DEVICE-FOUND"
#define P2P_DEVICE_LOST                          "P2P-DEVICE-LOST"
#define P2P_GROUP_FORMATION_SUCCESS              "P2P-GROUP-FORMATION-SUCCESS"
#define P2P_GROUP_REMOVED                        "P2P-GROUP-REMOVED"

class WpaSupplicantCommand
{
public:
    typedef std::function<void(QString)> ResponseCallback;

    WpaSupplicantCommand(const QString &message, ResponseCallback callback) :
        message(message),
        callback(callback)
    {
    }

    QString message;
    ResponseCallback callback;
};

class WpaSupplicantParser : public QObject
{
    Q_OBJECT
public:

    void feed(const QString &message)
    {
        if (message.size() == 0)
            return;

        if (message.startsWith('<')) {
            Q_EMIT unsolicitedResponse(message);
            return;
        }

        Q_EMIT solicitedResponse(message);
    }

Q_SIGNALS:
    void unsolicitedResponse(const QString &message);
    void solicitedResponse(const QString &message);
};

class WpaSupplicantCommandQueue : public QObject
{
    Q_OBJECT
public:
    WpaSupplicantCommandQueue(const QPointer<WpaSupplicantParser> &parser) :
        current(nullptr),
        parser(parser)
    {
        // Just forward all unsolicited response we get
        QObject::connect(parser, SIGNAL(unsolicitedResponse),
                         this, SIGNAL(onUnsolicitedResponse));

        QObject::connect(parser, SIGNAL(solicitedResponse(QString)),
                         this, SLOT(onSolicitedResponse(QString)));
    }

    void enqueueCommand(const QString &message, WpaSupplicantCommand::ResponseCallback callback)
    {
        queue.enqueue(new WpaSupplicantCommand(message, callback));
        restartQueue();
    }

Q_SIGNALS:
    void unsolicitedResponse(const QString &message);
    void transportWriteNeeded(const QString &message);

private:

    void restartQueue()
    {
        QTimer::singleShot(0, [=]() {
            checkRestartingQueue();
        });
    }

    void checkRestartingQueue()
    {
        if (current != nullptr || queue.size() == 0)
            return;

        writeNextCommand();
        restartQueue();
    }

    void writeNextCommand()
    {
        current = queue.dequeue();

        transportWriteNeeded(current->message);

        if (!current->callback) {
            delete current;
            current = nullptr;
        }
    }

private Q_SLOTS:

    void onSolicitedResponse(const QString &response)
    {
        if (current->callback)
            current->callback(response);

        delete current;
        // This will unblock the queue and will allow usto send the next command
        current = nullptr;

        restartQueue();
    }

private:
    WpaSupplicantCommand *current;
    QPointer<WpaSupplicantParser> parser;
    QQueue<WpaSupplicantCommand*> queue;
};

NetworkP2pManagerWpaSupplicant::NetworkP2pManagerWpaSupplicant(const QString &iface) :
    interface(iface),
    supplicantProcess(new QProcess(this)),
    ctrlPath(QString("/var/run/%1_supplicant").arg(interface)),
    dhcp(iface),
    parser(new WpaSupplicantParser),
    commandQueue(new WpaSupplicantCommandQueue(parser))
{
    QObject::connect(commandQueue, SIGNAL(transportWriteNeeded(QString)),
                     this, SLOT(onTransportWriteNeeded(QString)));
    QObject::connect(commandQueue, SIGNAL(unsolicitedResponse(QString)),
                     this, SLOT(onUnsolicitedResponse(QString)));

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
        qDebug() << "Attached!!";
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

int NetworkP2pManagerWpaSupplicant::bytesAvailableToRead()
{
    int nbytes = 0;
    // gives shorter than true amounts on Unix domain sockets.
    qint64 available = 0;
    if (ioctl(sock, FIONREAD, (char *) &nbytes) >= 0)
        available = (qint64) nbytes;
    return available;
}

void NetworkP2pManagerWpaSupplicant::onSocketReadyRead()
{
    char buf[READ_BUFFER_SIZE];

    while (bytesAvailableToRead() > 0) {

        int ret = recv(sock, buf, sizeof(buf) - 1, 0);
        if (ret < 0)
            return;

        buf[ret] = '\0';

        qDebug() << "IN:" << buf;

        parser->feed(QString(buf));
    }
}

void NetworkP2pManagerWpaSupplicant::onTransportWriteNeeded(const QString &message)
{
    qDebug() << "OUT:" << message;

    if (send(sock, message.toUtf8().constData(), message.size(), 0) < 0)
        qWarning() << "Failed to send data to wpa-supplicant";
}

void NetworkP2pManagerWpaSupplicant::request(const QString &command, std::function<void(QString)> callback)
{
    commandQueue->enqueueCommand(command, callback);
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

void NetworkP2pManagerWpaSupplicant::onUnsolicitedResponse(const QString &response)
{
    auto realResponse = response.mid(3);

    qDebug() << "Got unsolicited reply:" << realResponse;

    if (realResponse.startsWith(P2P_DEVICE_FOUND)) {
        // P2P-DEVICE-FOUND 4e:74:03:70:e2:c1 p2p_dev_addr=4e:74:03:70:e2:c1
        // pri_dev_type=8-0050F204-2 name='Aquaris M10' config_methods=0x188 dev_capab=0x5
        // group_capab=0x0 wfd_dev_info=0x00111c440032 new=1
        QStringList items = realResponse.split(QRegExp(" (?=[^']*('[^']*'[^']*)*$)"));

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
    else if (realResponse.startsWith(P2P_DEVICE_LOST)) {
        // P2P-DEVICE-LOST p2p_dev_addr=4e:74:03:70:e2:c1
        QStringList items = realResponse.split(" ");

        if (items.size() != 2)
            return;

        auto address = items[1].section('=', 1);
        peers.removeAll(address);
    }
    else if (realResponse.startsWith(P2P_GROUP_FORMATION_SUCCESS)) {
        qDebug() << "Starting DHCP client for interface" << interface;
        dhcp.start();
    }
    else if (realResponse.startsWith(P2P_GROUP_REMOVED)) {
        qDebug() << "Stopping DHCP client for interface" << interface;
        dhcp.stop();
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

int NetworkP2pManagerWpaSupplicant::connect(const QString &address, bool persistent)
{
    int ret = 0;

    auto cmd = QString("P2P_CONNECT %1 pbc %2")
                    .arg(address)
                    .arg(persistent ? "persistent" : "");

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
    int ret = 0;

    auto cmd = QString("P2P_GROUP_REMOVE %1").arg(interface);
    request(cmd, [&](const QString &result) {
        if (!checkResult(result)) {
            ret = -EIO;
            qWarning() << "Failed to disconnect all connected devices on interface" << interface;
            return;
        }
    });

    return ret;
}

#include "networkp2pmanagerwpasupplicant.moc"
