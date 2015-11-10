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

#include <QtGlobal>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QCommandLineParser>

#include <gst/gst.h>

#include "unixsignalhandler.h"
#include "miracastservice.h"
#include "miracastserviceadaptor.h"

static bool debuggingEnabled = false;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        if (debuggingEnabled)
            fprintf(stderr, "DEBUG: %s \n", localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "WARNING: %s \n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "CRITICAL: %s \n", localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "FATAL: %s \n", localMsg.constData());
        abort();
    }
}

int main(int argc, char **argv)
{
    qInstallMessageHandler(messageHandler);

    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Miracast service");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption debugOption("d", "Enable debugging output");
    parser.addOption(debugOption);

    parser.process(app);
    debuggingEnabled = parser.isSet(debugOption);

    gst_init(nullptr, nullptr);

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
