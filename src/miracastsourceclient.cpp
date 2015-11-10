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

#include <QHostAddress>

#include "miracastsourceclient.h"
#include "mirsourcemediamanager.h"

MiracastSourceClient::MiracastSourceClient(QTcpSocket *socket) :
    socket(socket)
{
    connect(socket, SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));

    mediaManager.reset(new MirSourceMediaManager);
    source.reset(wds::Source::Create(this, mediaManager.data()));

    source->Start();
}

MiracastSourceClient::~MiracastSourceClient()
{
}

void MiracastSourceClient::onSocketReadyRead()
{
    while (socket->bytesAvailable()) {
        auto data = socket->readAll();
        if (data.size() <= 0)
            break;

        qDebug() << "RTSP OUT:" << data;

        source->RTSPDataReceived(data.toStdString());
    }
}

void MiracastSourceClient::SendRTSPData(const std::string &data)
{
    qDebug() << "RTSP OUT:" << QString::fromStdString(data);

    if (socket->write(QString::fromStdString(data).toUtf8()) < 0) {
        qWarning() << "Failed to write data to RTSP client";
        return;
    }
}

std::string MiracastSourceClient::GetLocalIPAddress() const
{
    return socket->localAddress().toString().toStdString();
}

uint MiracastSourceClient::CreateTimer(int seconds)
{
    static unsigned int nextTimerId = 0;

    auto id = ++nextTimerId;
    auto timer = new QTimer;

    timer->setInterval(seconds * 1000);
    timer->setSingleShot(true);

    QObject::connect(timer, &QTimer::timeout, [=]() {
        // FIXME need the wds::Source for that
        source->OnTimerEvent(id);
    });

    timers.insert(id, timer);

    return id;
}

void MiracastSourceClient::ReleaseTimer(uint timerId)
{
    if (!timers.contains(timerId))
        return;

    auto timer = timers.value(timerId);
    timer->stop();
    timer->deleteLater();

    timers.remove(timerId);
}
