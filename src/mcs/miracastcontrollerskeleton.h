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
#include "aethercastinterface.h"
#ifdef __cplusplus
}
#endif

#include <memory>
#include <unordered_map>

#include "scoped_gobject.h"

#include "forwardingmiracastcontroller.h"
#include "networkdeviceadapter.h"

namespace mcs {
class MiracastControllerSkeleton : public std::enable_shared_from_this<MiracastControllerSkeleton>,
                                   public ForwardingMiracastController,
                                   public MiracastController::Delegate {
public:
    static constexpr const char *kBusName{"org.aethercast"};
    static constexpr const char *kManagerPath{"/org/aethercast"};
    static constexpr const char *kManagerIface{"org.aethercast.Manager"};

    static std::shared_ptr<MiracastControllerSkeleton> create(const std::shared_ptr<MiracastController> &controller);

    ~MiracastControllerSkeleton();

    void OnStateChanged(NetworkDeviceState state) override;
    void OnDeviceFound(const NetworkDevice::Ptr &device) override;
    void OnDeviceLost(const NetworkDevice::Ptr &device) override;
    void OnDeviceChanged(const NetworkDevice::Ptr &peer) override;
    void OnChanged() override;

private:
    static void OnNameAcquired(GDBusConnection *connection, const gchar *name, gpointer user_data);

    static gboolean OnHandleScan(AethercastInterfaceManager *skeleton, GDBusMethodInvocation *invocation,
                                 gpointer user_data);

    MiracastControllerSkeleton(const std::shared_ptr<MiracastController> &controller);
    std::shared_ptr<MiracastControllerSkeleton> FinalizeConstruction();

    void SyncProperties();

    std::string GenerateDevicePath(const NetworkDevice::Ptr &device) const;

private:
    ScopedGObject<AethercastInterfaceManager> manager_obj_;
    SharedGObject<GDBusConnection> bus_connection_;
    guint bus_id_;
    ScopedGObject<GDBusObjectManagerServer> object_manager_;
    std::unordered_map<std::string,NetworkDeviceAdapter::Ptr> devices_;
};
} // namespace mcs
#endif
