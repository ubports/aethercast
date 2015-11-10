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

#ifndef DHCPCLIENT_H_
#define DHCPCLIENT_H_

#include <QObject>
#include <QString>
#include <QScopedPointer>

class DhcpClient : public QObject
{
    Q_OBJECT
public:
    DhcpClient(const QString &interface);
    ~DhcpClient();

    bool start();
    void stop();

Q_SIGNALS:
    void addressAssigned(const QString &address);

private:
    class Private;
    QScopedPointer<Private> d;
};

#endif
