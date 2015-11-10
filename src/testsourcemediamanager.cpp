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

#include <gst/gst.h>

#include "testsourcemediamanager.h"

class TestSourceMediaManager::Private
{
public:
    QHostAddress remoteAddress;
};

TestSourceMediaManager::TestSourceMediaManager(const QHostAddress &remoteAddress) :
    d(new Private)
{
    d->remoteAddress = remoteAddress;
}

TestSourceMediaManager::~TestSourceMediaManager()
{
}

QString TestSourceMediaManager::constructPipeline()
{
    QString config;

    config += "videotestsrc ! videoconvert ! video/x-raw,format=I420 ! ";
    config += "x264enc ! muxer.  audiotestsrc ! avenc_ac3 ! muxer.  mpegtsmux name=muxer ! ";
    config += "rtpmp2tpay ! udpsink name=sink ";
    config += QString("host=%1 ").arg(d->remoteAddress.toString());
    config += QString("port=%1 ").arg(sinkPort1);

    qDebug() << "TestSourceMediaManager: " << config;

    return config;
}
