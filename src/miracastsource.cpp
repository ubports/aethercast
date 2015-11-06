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

#include "miracastsource.h"

MiracastSource::MiracastSource()
{
    QObject::connect(&server, SIGNAL(newConnection()),
                     this, SLOT(onNewConnection()));
}

MiracastSource::~MiracastSource()
{
}

bool MiracastSource::setup(const QString &address, quint16 port)
{
    if (server.isListening())
        return false;

    if (!server.listen(QHostAddress(address), port)) {
        qWarning() << "Failed to setup source RTSP server";
        return false;
    }

    return true;
}

void MiracastSource::onNewConnection()
{
    qDebug() << "Got a new connection";

    auto client = server.nextPendingConnection();

    // We stop here accepting any further connections as we only deal
    // with one at a time
    server.pauseAccepting();
}

void MiracastSource::release()
{
}
