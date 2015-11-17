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

#include "gstsourcemediamanager.h"

GstSourceMediaManager::GstSourceMediaManager() {
}

GstSourceMediaManager::~GstSourceMediaManager() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        g_source_remove(bus_watch_id_);
        gst_object_unref(GST_OBJECT(pipeline_));
    }
}

gboolean GstSourceMediaManager::OnGstBusEvent(GstBus *bus, GstMessage *message, gpointer data) {
    GError *err = NULL;
    gchar *debug = NULL;

    switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:
        gst_message_parse_error (message, &err, &debug);
        g_warning("GST ERROR: %s", err->message);
        g_error_free (err);
        g_free (debug);
        break;
    case GST_MESSAGE_WARNING:
        gst_message_parse_warning (message, &err, &debug);
        g_warning("GST WARNING: %s", err->message);
        g_error_free (err);
        g_free (debug);
        break;
    case GST_MESSAGE_INFO:
        gst_message_parse_info (message, &err, &debug);
        g_warning("GST INFO: %s", err->message);
        g_error_free (err);
        g_free (debug);
        break;
    default:
        /* unhandled message */
        break;
    }

    return TRUE;
}

void GstSourceMediaManager::Configure() {
    auto config = ConstructPipeline(format_);

    g_warning("pipeline: %s", config.c_str());

    GError *error = nullptr;
    pipeline_ = gst_parse_launch(config.c_str(), &error);
    if (error) {
        g_warning("Failed to setup GStreamer pipeline: %s", error->message);
        g_error_free(error);
        return;
    }

    auto bus = gst_pipeline_get_bus (GST_PIPELINE(pipeline_));
    bus_watch_id_ = gst_bus_add_watch(bus, &GstSourceMediaManager::OnGstBusEvent, this);
    gst_object_unref(bus);

    // Prepare pipeline so we're ready to go as soon as needed
    gst_element_set_state(pipeline_, GST_STATE_READY);
}

void GstSourceMediaManager::Play() {
    if (!pipeline_)
        return;

    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
}

void GstSourceMediaManager::Pause() {
    if (!pipeline_)
        return;

    gst_element_set_state(pipeline_, GST_STATE_PAUSED);
}

void GstSourceMediaManager::Teardown() {
    if (!pipeline_)
        return;

    gst_element_set_state(pipeline_, GST_STATE_READY);
}

bool GstSourceMediaManager::IsPaused() const {
    if (!pipeline_)
        return true;

    GstState state;
    gst_element_get_state(pipeline_, &state, nullptr, GST_CLOCK_TIME_NONE);

    return state != GST_STATE_PLAYING;
}
