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

#ifndef W11TNG_DHCPSERVER_H_
#define W11TNG_DHCPSERVER_H_

#include <boost/noncopyable.hpp>

#include <string>

#include <ac/glib_wrapper.h>

#include <ac/ip_v4_address.h>
#include <ac/non_copyable.h>

#include "processexecutor.h"
#include "filemonitor.h"

namespace w11tng {
class DhcpServer : public std::enable_shared_from_this<DhcpServer>,
                   public ProcessExecutor::Delegate,
                   public FileMonitor::Delegate {
public:
    typedef std::shared_ptr<DhcpServer> Ptr;

    class Delegate : private ac::NonCopyable {
    public:
        virtual void OnDhcpAddressAssigned(const ac::IpV4Address &local_address, const ac::IpV4Address &remote_address) = 0;
        virtual void OnDhcpTerminated() = 0;
    };

    static Ptr Create(const std::weak_ptr<Delegate> &delegate, const std::string &interface_name);

    ~DhcpServer();

    ac::IpV4Address LocalAddress() const;

    void OnProcessTerminated() override;
    void OnFileChanged(const std::string &path) override;

private:
    DhcpServer(const std::weak_ptr<Delegate> &delegate, const std::string &interface_name);

    void Start();

private:
    std::weak_ptr<Delegate> delegate_;
    std::string interface_name_;
    std::string lease_file_path_;
    std::string pid_file_path_;
    ProcessExecutor::Ptr executor_;
    FileMonitor::Ptr monitor_;
    ac::IpV4Address local_address_;
};
}
#endif
