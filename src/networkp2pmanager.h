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

#ifndef NETWORKP2PMANAGER_H_
#define NETWORKP2PMANAGER_H_

#include <QObject>
#include <QStringList>

class NetworkP2pManager : public QObject
{
	Q_OBJECT

public:
	virtual void setWfdSubElements(const QStringList &elements) = 0;
    enum State {
        Idle,
        Connecting,
        Connected,
        Disconnecting,
        Disconnected
    };

	virtual void scan(unsigned int timeout = 30) = 0;

	virtual QStringList getPeers() = 0;

	virtual int connect(const QString &address, bool persistent = true) = 0;
	virtual int disconnectAll() = 0;

    virtual State state() const = 0;

Q_SIGNALS:
	void peerFound(const QString &address);
	void peerLost(const QString &address);

	void groupStarted(const QString &address);
	void groupFormationFailed(const QString &address);
    void stateChanged();
};

#endif
