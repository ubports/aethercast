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

#ifndef MIRACAST_SERVICE_H_
#define MIRACAST_SERVICE_H_

#include <QObject>

#include "miracastsource.h"

class NetworkP2pManager;

class MiracastService : public QObject
{
    Q_OBJECT
public:
    MiracastService();
    ~MiracastService();

    NetworkP2pManager* networkManager() { return manager; }

private Q_SLOTS:
    void onSinkConnected(const QString &localAddress, const QString &remoteAddress);

private:
    NetworkP2pManager *manager;
    MiracastSource source;
};

#endif
