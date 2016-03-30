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

#ifndef W11TNG_HOSTNAME1STUB_H_
#define W11TNG_HOSTNAME1STUB_H_

#include <memory>
#include <string>

#include <mcs/glib_wrapper.h>

#include <mcs/scoped_gobject.h>

namespace w11tng {
class Hostname1Stub : public std::enable_shared_from_this<Hostname1Stub> {
public:
    typedef std::shared_ptr<Hostname1Stub> Ptr;

    static constexpr const char* kBusName{"org.freedesktop.hostname1"};
    static constexpr const char* kObjectPath{"/org/freedesktop/hostname1"};

    class Delegate {
    public:
        virtual void OnHostnameChanged() = 0;
    };

    static Ptr Create(const std::weak_ptr<Delegate> &delegate);

    ~Hostname1Stub();

    std::string Hostname() const;
    std::string StaticHostname() const;
    std::string PrettyHostname() const;
    std::string Chassis() const;

private:
    Hostname1Stub(const std::weak_ptr<Delegate> &delegate);
    Ptr FinalizeConstruction();

    void ParseProperties(GVariant *properties);
    void SyncProperties();

private:
    static void OnPropertiesChanged(GDBusConnection *connection, const gchar *sender_name,
                                    const gchar *object_path, const gchar *interface_name,
                                    const gchar *signal_name, GVariant *parameters,
                                    gpointer user_data);

private:
    std::weak_ptr<Delegate> delegate_;
    mcs::ScopedGObject<GDBusConnection> connection_;
    std::string hostname_;
    std::string static_hostname_;
    std::string pretty_hostname_;
    std::string chassis_;
};
} // namespace w11tng

#endif
