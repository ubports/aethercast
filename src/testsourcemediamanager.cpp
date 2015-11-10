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
    Private()
    {
    }

    ~Private()
    {
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            g_source_remove(busWatchId);
            gst_object_unref(GST_OBJECT(pipeline));
        }
    }

    static gboolean onGstBusEvent(GstBus *bus, GstMessage *message, gpointer data)
    {
        GError *err = NULL;
        gchar *debug = NULL;

        switch (GST_MESSAGE_TYPE (message)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error (message, &err, &debug);
            qWarning() << "GST ERROR:" << err->domain << ": " << err->message;
            g_error_free (err);
            g_free (debug);
            break;
        case GST_MESSAGE_WARNING:
            gst_message_parse_warning (message, &err, &debug);
            qWarning() << "GST WARNING:" << err->domain << ": " << err->message;
            g_error_free (err);
            g_free (debug);
            break;
        case GST_MESSAGE_INFO:
            gst_message_parse_info (message, &err, &debug);
            qWarning() << "GST INFO:" << err->domain << ": " << err->message;
            g_error_free (err);
            g_free (debug);
            break;
        default:
            /* unhandled message */
            break;
        }

        return TRUE;
    }

    GstElement *pipeline;
    QHostAddress remoteAddress;
    guint busWatchId;
};

TestSourceMediaManager::TestSourceMediaManager(const QHostAddress &remoteAddress) :
    d(new Private)
{
    d->remoteAddress = remoteAddress;
}

TestSourceMediaManager::~TestSourceMediaManager()
{
}

void TestSourceMediaManager::configure()
{
    QString config;
    config += "videotestsrc ! videoconvert ! video/x-raw,format=I420 ! ";
    config += "x264enc ! muxer.  audiotestsrc ! avenc_ac3 ! muxer.  mpegtsmux name=muxer ! ";
    config += "rtpmp2tpay ! udpsink name=sink ";
    config += QString("host=%1 ").arg(d->remoteAddress.toString());
    config += QString("port=%1 ").arg(sinkPort1);

    GError *error = nullptr;
    d->pipeline = gst_parse_launch(config.toUtf8().constData(), &error);
    if (error) {
        qWarning() << "Failed to setup GStreamer pipeline: " << error->message;
        g_error_free(error);
        return;
    }

    auto bus = gst_pipeline_get_bus (GST_PIPELINE(d->pipeline));
    d->busWatchId = gst_bus_add_watch(bus, &TestSourceMediaManager::Private::onGstBusEvent, this);
    gst_object_unref(bus);

    // Prepare pipeline so we're ready to go as soon as needed
    gst_element_set_state(d->pipeline, GST_STATE_READY);
}

void TestSourceMediaManager::Play()
{
    if (!d->pipeline)
        return;

    gst_element_set_state(d->pipeline, GST_STATE_PLAYING);
}

void TestSourceMediaManager::Pause()
{
    if (!d->pipeline)
        return;

    gst_element_set_state(d->pipeline, GST_STATE_PAUSED);
}

void TestSourceMediaManager::Teardown()
{
    if (!d->pipeline)
        return;

    gst_element_set_state(d->pipeline, GST_STATE_READY);
}

bool TestSourceMediaManager::IsPaused() const
{
    if (!d->pipeline)
        return true;

    GstState state;
    gst_element_get_state(d->pipeline, &state, nullptr, GST_CLOCK_TIME_NONE);

    return state != GST_STATE_PLAYING;
}
