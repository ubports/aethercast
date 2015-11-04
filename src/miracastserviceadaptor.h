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

#ifndef MIRACASTSERVICEADAPTOR_H_
#define MIRACASTSERVICEADAPTOR_H_

#include <QCoreApplication>
#include <QDBusAbstractAdaptor>

class MiracastService;

class MiracastServiceAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.ubuntu.miracast.Manager")
    Q_PROPERTY(QString state READ state)
    Q_PROPERTY(bool powered READ powered)

public:
    MiracastServiceAdaptor(QCoreApplication *app, MiracastService *service);

    QString state() { return QString("disconnected"); }
    bool powered() { return false; }

public Q_SLOTS:
    void FindPeers();
    QStringList GetPeers();
    void Connect(const QString &address);

private:
    MiracastService *service;
};

#endif
