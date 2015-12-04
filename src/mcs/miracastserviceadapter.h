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

#include <memory>

#include "scoped_gobject.h"

#include "miracastservice.h"
#include "networkdeviceadapter.h"

namespace mcs {
class MiracastServiceAdapter : public std::enable_shared_from_this<MiracastServiceAdapter>,
                               public MiracastService::Delegate {
public:
    static constexpr const char *kBusName{"org.wds"};
    static constexpr const char *kManagerPath{"/"};
    static constexpr const char *kManagerIface{"org.wds.Manager"};

    static std::shared_ptr<MiracastServiceAdapter> create(const std::shared_ptr<MiracastService> &service);

    ~MiracastServiceAdapter();

    void OnStateChanged(NetworkDeviceState state) override;
    void OnDeviceFound(const NetworkDevice::Ptr &device) override;
    void OnDeviceLost(const NetworkDevice::Ptr &device) override;

private:
    static void OnNameAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data);

    static void OnHandleScan(MiracastInterfaceManager *skeleton, GDBusMethodInvocation *invocation,
                              gpointer user_data);
    static void OnHandleConnectSink(MiracastInterfaceManager *skeleton, GDBusMethodInvocation *invocation,
                                      const gchar *address, gpointer user_data);

    MiracastServiceAdapter(const std::shared_ptr<MiracastService> &service);
    std::shared_ptr<MiracastServiceAdapter> FinalizeConstruction();
private:
    std::shared_ptr<MiracastService> service_;
    ScopedGObject<MiracastInterfaceManager> manager_obj_;
    GDBusConnection *bus_connection_;
    guint bus_id_;
    ScopedGObject<GDBusObjectManagerServer> object_manager_;
    std::map<std::string,NetworkDeviceAdapter::Ptr> devices_;
};
} // namespace mcs
#endif
