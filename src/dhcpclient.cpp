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

#include <QDebug>

#include "dhcpclient.h"

#define DHCLIENT_BIN_PATH   "/sbin/dhclient"

DhcpClient::DhcpClient(const QString &interface) :
    interface(interface)
{
}

DhcpClient::~DhcpClient()
{
}

void DhcpClient::start()
{
    process = new QProcess(this);

    QObject::connect(process, SIGNAL(error(QProcess::ProcessError)),
                     this, SLOT(onProcessError(QProcess::ProcessError)));
    QObject::connect(process, SIGNAL(finished(int)),
                     this, SLOT(onProcessFinished(int)));

    QStringList arguments;
    // Run in foreground so we can control the process
    arguments << "-d";
    arguments << interface;

    process->start(DHCLIENT_BIN_PATH, arguments);
    process->waitForStarted();

    if (process->state() != QProcess::Running) {
        qWarning() << "Failed to start dhclient for interface" << interface;
        return;
    }
}

void DhcpClient::onProcessError(QProcess::ProcessError error)
{
}

void DhcpClient::onProcessFinished(int errorCode)
{
}

void DhcpClient::stop()
{
    if (!process)
        return;

    process->close();

    process->deleteLater();
    process = nullptr;
}
