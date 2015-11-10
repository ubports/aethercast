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

class NullSourceMediaManager : public BaseSourceMediaManager
{
public:
    void Play() override
    {
        qWarning() << "NullSourceMediaManager: Not implemented";
    }

    void Pause() override
    {
        qWarning() << "NullSourceMediaManager: Not implemented";
    }

    void Teardown() override
    {
        qWarning() << "NullSourceMediaManager: Not implemented";
    }

    bool IsPaused() const override
    {
        qWarning() << "NullSourceMediaManager: Not implemented";
    }

protected:
    void configure() override
    {
    }
};

BaseSourceMediaManager* MediaManagerFactory::createSource(const QHostAddress &remoteAddress)
{
    auto type = qgetenv("MIRACAST_SOURCE_TYPE");

    qDebug() << "Creating source media manager of type" << type;

    if (type.size() == 0 || type == "mir")
        return new MirSourceMediaManager(remoteAddress);
    else if (type == "test")
        return new TestSourceMediaManager(remoteAddress);

    return new NullSourceMediaManager();
}
