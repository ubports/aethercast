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

#include <sys/signalfd.h>

#include <gst/gst.h>

#include "miracastservice.h"
#include "miracastserviceadapter.h"

#define VERSION     "0.1"

static gboolean option_debug = FALSE;
static gboolean option_version = FALSE;

static GMainLoop *main_loop = nullptr;

static unsigned int __terminated = 0;

static GOptionEntry options[] = {
    { "debug", 'd', 0, G_OPTION_ARG_NONE, &option_debug,
                "Enable debugging mode" },
    { "version", 'v', 0, G_OPTION_ARG_NONE, &option_version,
                "Show version information and exit" },
    { NULL },
};

int main(int argc, char **argv) {
    GOptionContext *context;
    GError *error = nullptr;

    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, options, NULL);

    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        if (error) {
            g_printerr("%s\n", error->message);
            g_error_free(error);
        } else
            g_printerr("An unknown error occurred\n");
        exit(1);
    }

    g_option_context_free(context);

    if (option_version) {
        printf("%s\n", VERSION);
        return 0;
    }

    gst_init(nullptr, nullptr);

    main_loop = g_main_loop_new(nullptr, FALSE);

    MiracastService service(nullptr);
    MiracastServiceAdapter adapter(&service);

    g_main_loop_run(main_loop);

    g_main_loop_unref(main_loop);

    return 0;
}
