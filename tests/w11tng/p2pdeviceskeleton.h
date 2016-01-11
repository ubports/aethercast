/*
 * Copyright (C) 2016 Canonical, Ltd.
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

#ifndef W11TNG_TESTING_P2PDEVICE_SKELETON_H_
#define W11TNG_TESTING_P2PDEVICE_SKELETON_H_

#include <mcs/non_copyable.h>

extern "C" {
#include "wpasupplicantinterface.h"
}

#include "baseskeleton.h"

namespace w11tng {
namespace testing {

class P2PDeviceSkeleton : public std::enable_shared_from_this<P2PDeviceSkeleton>,
        public BaseSkeleton<WpaSupplicantInterfaceP2PDevice> {
public:
    typedef std::shared_ptr<P2PDeviceSkeleton> Ptr;

    class Delegate : public mcs::NonCopyable {
    public:
        virtual void OnFind() = 0;
        virtual void OnStopFind() = 0;
    };

    static Ptr Create(const std::string &object_path);

    ~P2PDeviceSkeleton();

    void SetDelegate(const std::weak_ptr<Delegate> &delegate);

    void EmitDeviceFound(const std::string &path);
    void EmitDeviceLost(const std::string &path);

private:
    P2PDeviceSkeleton(const std::string &object_path);
    Ptr FinalizeConstruction();

private:
    static gboolean OnHandleFind(WpaSupplicantInterfaceP2PDevice *device, GDBusMethodInvocation *invocation, GVariant *properties, gpointer user_data);

private:
    std::weak_ptr<Delegate> delegate_;
};

} // namespace testing
} // namespace w11tng

#endif
