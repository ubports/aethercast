/*
 * Copyright (C) 2014 Canonical, Ltd.
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
 * Author: Pete Woods <pete.woods@canonical.com>
 */

#ifndef UNIXSIGNALHANDLER_H_
#define UNIXSIGNALHANDLER_H_

#include <QObject>
#include <QSocketNotifier>
#include <functional>

namespace util
{

class UnixSignalHandler: public QObject {
Q_OBJECT

public:
    UnixSignalHandler(const std::function<void()>& f, QObject *parent = 0);

    ~UnixSignalHandler() = default;

    static int setupUnixSignalHandlers();

protected Q_SLOTS:
    void handleSigInt();

    void handleSigTerm();

protected:
    static void intSignalHandler(int unused);

    static void termSignalHandler(int unused);

    static int sigintFd[2];

    static int sigtermFd[2];

    std::function<void()> m_func;

    QSocketNotifier *m_socketNotifierInt;

    QSocketNotifier *m_socketNotifierTerm;
};

}

#endif
