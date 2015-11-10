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

#ifndef MIRACASTSOURCE_H_
#define MIRACASTSOURCE_H_

#include <QObject>
#include <QTcpServer>

class MiracastSourceClient;

class MiracastSource : public QObject
{
    Q_OBJECT
public:
    MiracastSource();
    ~MiracastSource();

    bool setup(const QString &address, quint16 port);
    void release();

Q_SIGNALS:
    void clientDisconnected();

private Q_SLOTS:
    void onNewConnection();

private:
    QTcpServer server;
    MiracastSourceClient *currentClient;
};

#endif
