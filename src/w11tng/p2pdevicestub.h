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

#ifndef W11TNG_P2P_DEVICE_STUB_H_
#define W11TNG_P2P_DEVICE_STUB_H_

#include <chrono>
#include <memory>
#include <unordered_map>

#include <mcs/non_copyable.h>

#include <gio/gio.h>

extern "C" {
#include "wpasupplicantinterface.h"
}

#include <mcs/shared_gobject.h>
#include <mcs/scoped_gobject.h>

#include "networkdevice.h"

namespace w11tng {

class P2PDeviceStub : public std::enable_shared_from_this<P2PDeviceStub>{
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};

    class Delegate : public mcs::NonCopyable {
    public:
        virtual void OnDeviceFound(const std::string &path) = 0;
        virtual void OnDeviceLost(const std::string &path) = 0;

        virtual void OnGroupOwnerNegotiationSuccess(const std::string &peer_path) = 0;
        virtual void OnGroupOwnerNegotiationFailure(const std::string &peer_path) = 0;
        virtual void OnGroupStarted(const std::string &group_path, const std::string &interface_path, const std::string &role) = 0;
        virtual void OnGroupFinished(const std::string &group_path, const std::string &interface_path) = 0;
        virtual void OnGroupRequest(const std::string &peer_path) = 0;

        // Called whenver any of the exposed properties changes.
        virtual void OnP2PDeviceChanged() = 0;

        virtual void OnP2PDeviceReady() = 0;
    };

    typedef std::shared_ptr<P2PDeviceStub> Ptr;

    static P2PDeviceStub::Ptr Create(const std::string &object_path, const std::weak_ptr<P2PDeviceStub::Delegate> &delegate);

    ~P2PDeviceStub();

    void Find(const std::chrono::seconds &timeout);
    void StopFind();
    bool Connect(const std::string &path);
    bool Disconnect();
    bool DisconnectSync();
    void Flush();
    void Cancel();

    bool Scanning() const { return scan_timeout_source_ > 0; }
    bool Connected() const { return !!proxy_; }

private:
    static void OnDeviceFound(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, gpointer user_data);
    static void OnDeviceLost(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, gpointer user_data);

    static void OnGONegotiationSuccess(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data);
    static void OnGONegotiationFailure(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data);
    static void OnGroupStarted(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data);
    static void OnGroupFinished(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data);
    static void OnGroupRequest(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data);

private:
    P2PDeviceStub(const std::weak_ptr<P2PDeviceStub::Delegate> &delegate);
    std::shared_ptr<P2PDeviceStub> FinalizeConstruction(const std::string &object_path);

    void ConnectSignals();

    void StartFindTimeout();
    void StopFindTimeout();

private:
    std::weak_ptr<P2PDeviceStub::Delegate> delegate_;
    mcs::ScopedGObject<GDBusConnection> connection_;
    mcs::ScopedGObject<WpaSupplicantInterfaceP2PDevice> proxy_;
    std::chrono::seconds scan_timeout_;
    guint scan_timeout_source_;
    std::unordered_map<std::string,w11tng::NetworkDevice::Ptr> devices_;
};

} // namespace w11tng

#endif
