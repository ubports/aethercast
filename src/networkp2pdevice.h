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

#ifndef NETWORKP2PDEVICE_H_
#define NETWORKP2PDEVICE_H_

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QSharedPointer>

class WfdDeviceInfo;

class NetworkP2pDevice : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<NetworkP2pDevice> Ptr;

    enum State {
        Idle,
        Failure,
        Connecting,
        Connected,
        Disconnected
    };

    enum Role {
        Undecied,
        GroupOwner,
        GroupClient
    };

    static QString stateToStr(State state);

    NetworkP2pDevice();
    ~NetworkP2pDevice();

    QString address() const;
    QString name() const;
    State state() const;
    QString stateAsString() const;
    WfdDeviceInfo wfdDeviceInfo() const;
    int configMethods() const;
    Role role() const;

    void setAddress(const QString &address);
    void setName(const QString &name);
    void setState(State state);
    void setWfdDeviceInfo(const WfdDeviceInfo &wfdDeviceInfo);
    void setConfigMethods(int configMethods);
    void setRole(Role role);

private:
    class Private;
    QScopedPointer<Private> d;
};

#endif
