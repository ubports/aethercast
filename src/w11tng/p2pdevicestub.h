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
    static constexpr const char *kManagerPath{"/fi/w1/wpa_supplicant1"};

    class Delegate : public mcs::NonCopyable {
    public:
        // Will be called as soon as the stub has connected with supplicant
        // and found a suitable already existing interface to use for P2P.
        virtual void OnSystemReady() = 0;

        // If the interface we're using is removed due to any reason the
        // device will kill itself and waits for an suitable interface to
        // appear again.
        virtual void OnSystemKilled() = 0;

        virtual void OnDeviceFound(const std::string &path) = 0;
        virtual void OnDeviceLost(const std::string &path) = 0;

        // Called whenver any of the exposed properties changes.
        virtual void OnChanged() = 0;
    };

    typedef std::shared_ptr<P2PDeviceStub> Ptr;

    static P2PDeviceStub::Ptr Create(const std::weak_ptr<P2PDeviceStub::Delegate> &delegate);

    ~P2PDeviceStub();

    bool IsP2PSupport() const;

    void Find(const std::chrono::seconds &timeout);
    void StopFind();
    bool Connect(const w11tng::NetworkDevice::Ptr &device);
    bool Disconnect();

    bool Scanning() const { return scan_timeout_source_ > 0; }
    bool Connected() const { return !!p2p_device_; }

private:
    static void OnServiceLost(GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void OnServiceFound(GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data);

    static void OnDeviceFound(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, gpointer user_data);
    static void OnDeviceLost(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, gpointer user_data);

    static void OnFindStopped(gpointer user_data);

private:
    P2PDeviceStub(const std::weak_ptr<P2PDeviceStub::Delegate> &delegate);
    std::shared_ptr<P2PDeviceStub> FinalizeConstruction();

    void Initialize();
    void InitializeP2P();
    void Reset();
    void ConnectSignals();

    void StartFindTimeout();

private:
    std::weak_ptr<P2PDeviceStub::Delegate> delegate_;
    mcs::ScopedGObject<GDBusConnection> connection_;
    mcs::ScopedGObject<WpaSupplicantFiW1Wpa_supplicant1> manager_;
    mcs::ScopedGObject<WpaSupplicantInterface> iface_;
    mcs::ScopedGObject<WpaSupplicantInterfaceP2PDevice> p2p_device_;
    bool p2p_supported_;
    std::chrono::seconds scan_timeout_;
    guint scan_timeout_source_;
    std::unordered_map<std::string,w11tng::NetworkDevice::Ptr> devices_;
};

} // namespace w11tng

#endif
