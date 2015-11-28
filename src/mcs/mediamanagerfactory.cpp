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

#include "mediamanagerfactory.h"
#include "mirsourcemediamanager.h"
#include "testsourcemediamanager.h"

namespace mcs {
class NullSourceMediaManager : public BaseSourceMediaManager
{
public:
    void Play() override {
        g_warning("NullSourceMediaManager: Not implemented");
    }

    void Pause() override {
        g_warning("NullSourceMediaManager: Not implemented");
    }

    void Teardown() override {
        g_warning("NullSourceMediaManager: Not implemented");
    }

    bool IsPaused() const override {
        g_warning("NullSourceMediaManager: Not implemented");
    }

protected:
    void Configure() override {
        g_warning("NullSourceMediaManager: Not implemented");
    }
};

BaseSourceMediaManager* MediaManagerFactory::CreateSource(const std::string &remote_address) {
    auto type = getenv("MIRACAST_SOURCE_TYPE");

    g_warning("Creating source media manager of type %s", type);

    if (!type)
        return new NullSourceMediaManager();

    if (g_strcmp0(type, "mir") == 0)
        return new MirSourceMediaManager(remote_address);
    else if (g_strcmp0(type, "test") == 0)
        return new TestSourceMediaManager(remote_address);

    return new NullSourceMediaManager();
}
} // namespace mcs
