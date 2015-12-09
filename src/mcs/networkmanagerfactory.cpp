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

#include <boost/concept_check.hpp>

#include "networkmanagerfactory.h"
#include "wpasupplicantnetworkmanager.h"

namespace mcs {

NetworkManager::Ptr NetworkManagerFactory::Create(const std::string &type) {
    boost::ignore_unused_variable_warning(type);

    // FIXME for now we only can create the wpa one but this will be extended
    // with further types.
    return std::make_shared<WpaSupplicantNetworkManager>();
}

} // namespace mcs
