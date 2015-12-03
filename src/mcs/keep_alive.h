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
#ifndef KEEP_ALIVE_H_
#define KEEP_ALIVE_H_

#include <memory>

namespace mcs {
namespace detail {
template<typename T, template<typename> class Ptr>
class KeepAlive {
public:
    KeepAlive(const Ptr<T>& inst) : inst_(inst) {
    }

    const Ptr<T>& GetInstance() const {
        return inst_;
    }

    Ptr<T> ShouldDie() {
        auto inst = inst_;
        delete this;
        return inst;
    }

private:
    Ptr<T> inst_;
};
}
// SharedKeepAlive helps in delivering an object securely through a callback-based
// system that takes context as a void* (e.g., the glib calls in this class).
// A SharedKeepAlive<T> instance simply wraps a managed ptr to an instance of T and keeps
// it alive while an async operation is in progress. ShouldDie() is just a concise way of
// unwrapping the instance and cleaning up the SharedKeepAlive<T> instance itself.
template<typename T>
using SharedKeepAlive = detail::KeepAlive<T, std::shared_ptr>;

// WeekKeepAlive helps in delivering an object securely through a callback-based
// system that takes context as a void* (e.g., the glib calls in this class).
// A WeekKeepAlive<T> instance simply wraps a weak managed ptr to an instance of T and keeps
// it alive while an async operation is in progress. It is specifically meant for pub-sub
// setups, with the callback staying alive across invocations.
template<typename T>
using WeakKeepAlive = detail::KeepAlive<T, std::weak_ptr>;
}

#endif // KEEP_ALIVE_H_
