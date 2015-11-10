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

#ifndef MIRACASTSOURCECLIENT_H_
#define MIRACASTSOURCECLIENT_H_

#include <string>

#include <QObject>
#include <QScopedPointer>
#include <QTimer>
#include <QTcpSocket>

#include <wds/peer.h>
#include <wds/source.h>
#include <wds/media_manager.h>

class MiracastSourceClient : public QObject,
                             public wds::Peer::Delegate
{
    Q_OBJECT
public:
    MiracastSourceClient(QTcpSocket *socket);
    ~MiracastSourceClient();

public:
    void SendRTSPData(const std::string &data) override;
    std::string GetLocalIPAddress() const override;
    uint CreateTimer(int seconds) override;
    void ReleaseTimer(uint timerId) override;

Q_SIGNALS:
    void connectionClosed();

private Q_SLOTS:
    void onSocketReadyRead();
    void onSocketDisconnected();

private:
    void dumpRtsp(const QString &prefix, const QString &data);

private:
    QTcpSocket *socket;
    QMap<int,QTimer*> timers;
    QScopedPointer<wds::Source> source;
    QScopedPointer<wds::SourceMediaManager> mediaManager;
};

#endif
