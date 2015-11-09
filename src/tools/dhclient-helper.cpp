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

#include <QLocalSocket>
#include <QProcessEnvironment>
#include <QString>

QStringList itemsToIgnore = { };

int main(int argc, char **argv)
{
    QLocalSocket socket;
    socket.connectToServer("/var/run/miracast-service/private-dhcp", QIODevice::WriteOnly);

    auto environ = QProcessEnvironment::systemEnvironment();

    for (auto name : environ.keys()) {
        if (itemsToIgnore.contains(name))
            continue;

        auto line = QString("%1=%2")
                .arg(name)
                .arg(environ.value(name));

        qDebug() << line;

        socket.write(line.toUtf8());
    }

    socket.waitForBytesWritten();
    socket.flush();
    socket.close();

    return 0;
}
