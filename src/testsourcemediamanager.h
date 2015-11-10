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

#ifndef TESTMEDIAMANAGER_H_
#define TESTMEDIAMANAGER_H_

#include <QScopedPointer>
#include <QHostAddress>

#include "basesourcemediamanager.h"

class TestSourceMediaManager : public BaseSourceMediaManager
{
public:
    explicit TestSourceMediaManager(const QHostAddress &remoteAddress);
    ~TestSourceMediaManager();

    void Play() override;
    void Pause() override;
    void Teardown() override;
    bool IsPaused() const override;

protected:
    void configure() override;

private:
    class Private;
    QScopedPointer<Private> d;
};

#endif
