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

#ifndef W11TNG_URFKILL_MANAGER_H_
#define W11TNG_URFKILL_MANAGER_H_

#include "w11tng/rfkillmanager.h"
#include "ac/scoped_gobject.h"

#include <map>

namespace w11tng {
class URfkillManager : public RfkillManager,
                       public std::enable_shared_from_this<URfkillManager> {
public:
    static Ptr Create();

    static constexpr const char* kBusName{"org.freedesktop.URfkill"};
    static constexpr const char* kObjectPath{"/org/freedesktop/URfkill/WLAN"};

    ~URfkillManager();

    bool IsBlocked(const Type &type) const override;

private:
    URfkillManager();
    Ptr FinalizeConstruction();

    void ParseProperties(GVariant *properties);
    void SyncProperties();

    static void OnPropertiesChanged(GDBusConnection *connection, const gchar *sender_name,
                                    const gchar *object_path, const gchar *interface_name,
                                    const gchar *signal_name, GVariant *parameters,
                                    gpointer user_data);

    ac::ScopedGObject<GDBusConnection> connection_;
    std::map<Type,bool> block_status_;
};
} // namespace w11tng

#endif
