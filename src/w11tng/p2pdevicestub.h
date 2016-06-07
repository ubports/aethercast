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
#include <set>

#include <ac/non_copyable.h>

#include <ac/glib_wrapper.h>

extern "C" {
// Ignore all warnings coming from the external headers as we don't
// control them and also don't want to get any warnings from them
// which will only pollute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include "wpasupplicantinterface.h"
#pragma GCC diagnostic pop
}

#include <ac/shared_gobject.h>
#include <ac/scoped_gobject.h>

#include "networkdevice.h"

namespace w11tng {

class P2PDeviceStub : public std::enable_shared_from_this<P2PDeviceStub>{
public:
    static constexpr const char *kBusName{"fi.w1.wpa_supplicant1"};

    typedef int Frequency;
    typedef std::set<Frequency> FrequencyList;

    enum class Status {
        kSuccess = 0,
        kInformationIsCurrentlyUnavailable = 1,
        kIncompatibleParameters = 2,
        kLimitReached = 3,
        kInvalidParameter = 4,
        kUnableToAccommodateRequest = 5,
        kProtcolErrorOrDisruptiveBehavior = 6,
        kNoCommonChannel = 7,
        kUnknownP2PGroup = 8,
        kBothGOIntent15 = 9,
        kIncompatibleProvisioningMethod = 10,
        kRejectByUser = 11,
        kSucccesAcceptedByUser = 12,
        kUnknown = 0xff
    };

    static std::string StatusToString(Status status);

    enum class Property {
        kStatus,
        kPeerObject,
        kFrequency,
        kFrequencyList,
        kWpsMethod
    };

    static std::string PropertyToString(Property property);

    enum class WpsMethod {
        kPbc,
        kPin
    };

    static WpsMethod WpsMethodFromString(const std::string &wps_method);
    static std::string WpsMethodToString(WpsMethod wps_method);

    struct GroupOwnerNegotiationResult {
        GroupOwnerNegotiationResult() { }
        GroupOwnerNegotiationResult(const GroupOwnerNegotiationResult &other) :
            status(other.status),
            oper_freq(other.oper_freq),
            frequencies(other.frequencies),
            wps_method(other.wps_method) {
        }

        Status status;
        Frequency oper_freq;
        FrequencyList frequencies;
        WpsMethod wps_method;
    };

    class Delegate : public ac::NonCopyable {
    public:
        virtual void OnDeviceFound(const std::string &path) = 0;
        virtual void OnDeviceLost(const std::string &path) = 0;

        virtual void OnPeerConnectFailed() = 0;
        virtual void OnGroupOwnerNegotiationSuccess(const std::string &peer_path, const GroupOwnerNegotiationResult &result) = 0;
        virtual void OnGroupOwnerNegotiationFailure(const std::string &peer_path, const GroupOwnerNegotiationResult &result) = 0;
        virtual void OnGroupStarted(const std::string &group_path, const std::string &interface_path, const std::string &role) = 0;
        virtual void OnGroupFinished(const std::string &group_path, const std::string &interface_path) = 0;
        virtual void OnGroupRequest(const std::string &peer_path, int dev_passwd_id) = 0;

        // Called whenver any of the exposed properties changes.
        virtual void OnP2PDeviceChanged() = 0;

        virtual void OnP2PDeviceReady() = 0;
    };

    typedef std::shared_ptr<P2PDeviceStub> Ptr;

    static P2PDeviceStub::Ptr Create(const std::string &object_path, const std::weak_ptr<P2PDeviceStub::Delegate> &delegate);

    ~P2PDeviceStub();

    void Find(const std::chrono::seconds &timeout);
    void StopFind();
    bool Connect(const std::string &path, const std::int32_t intent);
    bool Disconnect();
    bool DisconnectSync();
    void Flush();
    void Cancel();

    bool Scanning() const { return scan_timeout_source_ > 0; }
    bool Connected() const { return !!proxy_; }
    std::string ObjectPath() const;

    void SetDeviceConfiguration(const std::string &device_name, const std::string &device_type);

private:
    static void OnDeviceFound(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, gpointer user_data);
    static void OnDeviceLost(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, gpointer user_data);

    static void OnGONegotiationSuccess(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data);
    static void OnGONegotiationFailure(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data);
    static void OnGroupStarted(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data);
    static void OnGroupFinished(WpaSupplicantInterfaceP2PDevice *device, GVariant *properties, gpointer user_data);
    static void OnGroupRequest(WpaSupplicantInterfaceP2PDevice *device, const gchar *path, int dev_passwd_id, gpointer user_data);

private:
    P2PDeviceStub(const std::weak_ptr<P2PDeviceStub::Delegate> &delegate);
    std::shared_ptr<P2PDeviceStub> FinalizeConstruction(const std::string &object_path);

    void ConnectSignals();

    void StartFindTimeout();
    void StopFindTimeout();

private:
    std::weak_ptr<P2PDeviceStub::Delegate> delegate_;
    ac::ScopedGObject<GDBusConnection> connection_;
    ac::ScopedGObject<WpaSupplicantInterfaceP2PDevice> proxy_;
    std::chrono::seconds scan_timeout_;
    guint scan_timeout_source_;
    std::unordered_map<std::string,w11tng::NetworkDevice::Ptr> devices_;
};

} // namespace w11tng

#endif
