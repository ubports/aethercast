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

#include <atomic>

#include <gst/gst.h>

#include "initgstreameronce.h"
#include "logger.h"

namespace {
void GstLog (GstDebugCategory *category,
          GstDebugLevel level,
          const gchar *file,
          const gchar *function,
          gint line,
          GObject *object,
          GstDebugMessage *message,
          gpointer user_data) {

    boost::optional<mcs::Logger::Location> location;
    if (file && function && line > 0) {
        location = mcs::Logger::Location{file, function, line};
    }

    mcs::Log().Log(mcs::GstDebugLevelToSeverity(level), gst_debug_message_get(message), location);
}
}
namespace mcs {
Logger::Severity GstDebugLevelToSeverity(GstDebugLevel level) {
    switch (level) {
    case GST_LEVEL_NONE:
    case GST_LEVEL_ERROR:
        return Logger::Severity::kError;
    case GST_LEVEL_WARNING:
    case GST_LEVEL_FIXME:
        return Logger::Severity::kWarning;
    case GST_LEVEL_INFO:
        return Logger::Severity::kInfo;
    case GST_LEVEL_DEBUG:
        return Logger::Severity::kDebug;
    case GST_LEVEL_LOG:
        return Logger::Severity::kDebug;
    case GST_LEVEL_TRACE:
    case GST_LEVEL_MEMDUMP:
        return Logger::Severity::kTrace;
    default:
        return Logger::Severity::kInfo;
    }
}

void InitGstreamerOnceOrThrow() {
    static std::atomic<bool> initialized(false);
    if (initialized.exchange(true))
        return;

    GError* error = nullptr;
    if (gst_init_check(nullptr, nullptr, &error) == FALSE) {
        auto what = Utils::Sprintf("Failed to initialize gstreamer (%s: %s)", g_quark_to_string(error->domain), error->message);
        g_error_free(error);
        throw std::runtime_error{what};
    }

    // Get rid of gstreamer's default log function.
    gst_debug_remove_log_function(nullptr);
    // And install our own.
    gst_debug_add_log_function(GstLog, nullptr, nullptr);
    // No need to, our logging infra takes care of that, too.
    gst_debug_set_colored(FALSE);
    auto gst_debug = Utils::GetEnvValue("AETHERCAST_GST_DEBUG");
    if (not gst_debug.empty())
        gst_debug_set_threshold_from_string(gst_debug.c_str(), FALSE);
}
}
