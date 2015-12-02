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

#include <string.h>

#include "mediamanagerfactory.h"
#include "mirsourcemediamanager.h"
#include "testsourcemediamanager.h"

namespace mcs {

void NullSourceMediaManager::Play() {
    g_warning("NullSourceMediaManager: Not implemented");
}

void NullSourceMediaManager::Pause() {
    g_warning("NullSourceMediaManager: Not implemented");
}

void NullSourceMediaManager::Teardown() {
    g_warning("NullSourceMediaManager: Not implemented");
}

bool NullSourceMediaManager::IsPaused() const {
    g_warning("NullSourceMediaManager: Not implemented");
}

void NullSourceMediaManager::Configure() {
    g_warning("NullSourceMediaManager: Not implemented");
}

std::shared_ptr<BaseSourceMediaManager> MediaManagerFactory::CreateSource(const std::string &remote_address) {
    std::string type = getenv("MIRACAST_SOURCE_TYPE");

    g_warning("Creating source media manager of type %s", type.c_str());

    if (type.length() == 0 || type == "mir")
        return std::make_shared<MirSourceMediaManager>(remote_address);
    else if (type == "test")
        return TestSourceMediaManager::create(remote_address);

    return std::make_shared<NullSourceMediaManager>();
}
} // namespace mcs
