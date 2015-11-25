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

#ifndef MIRACASTSERVICEADAPTOR_H_
#define MIRACASTSERVICEADAPTOR_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "miracastinterface.h"
#ifdef __cplusplus
}
#endif

#define MIRACAST_SERVICE_BUS_NAME       "com.canonical.miracast"

#define MIRACAST_SERVICE_MANAGER_PATH   "/"
#define MIRACAST_SERVICE_MANAGER_IFACE  "com.canonical.miracast.Manager"

class MiracastService;

class MiracastServiceAdapter {
public:
    MiracastServiceAdapter(MiracastService *service);
    ~MiracastServiceAdapter();

private:
    static void OnNameAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data);

    static void OnHandleScan(MiracastInterfaceManager *skeleton, GDBusMethodInvocation *invocation,
                              gpointer user_data);
    static void OnHandleConnectSink(MiracastInterfaceManager *skeleton, GDBusMethodInvocation *invocation,
                                      const gchar *address, gpointer user_data);

private:
    MiracastService *service_;
    MiracastInterfaceManager *manager_obj_;
    guint bus_id_;
    GDBusObjectManagerServer *object_manager_;
};

#endif
