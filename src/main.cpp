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

#include <QCoreApplication>
#include <QDBusConnection>

#include "unixsignalhandler.h"
#include "miracastservice.h"
#include "miracastserviceadaptor.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    util::UnixSignalHandler handler([]{
        QCoreApplication::exit(0);
    });
    handler.setupUnixSignalHandlers();

    MiracastService service;
    new MiracastServiceAdaptor(&service);

    QDBusConnection::systemBus().registerService("com.ubuntu.miracast");
    QDBusConnection::systemBus().registerObject("/", &service);

    return app.exec();
}
