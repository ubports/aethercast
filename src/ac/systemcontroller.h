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

#ifndef AC_SYSTEMCONTROLLER_H_
#define AC_SYSTEMCONTROLLER_H_

#include <memory>

namespace ac {

enum class DisplayState {
    // The display is off.
    Off = 0,
    // The display is on.
    On = 1,
};

class SystemController {
public:
    typedef std::shared_ptr<SystemController> Ptr;

    template<typename State>
    struct Lock {
        typedef std::shared_ptr<Lock> Ptr;

        class Delegate {
        public:
            virtual void OnLockAcquired(State state);
            virtual void OnLockReleased(State state);
        };

        Lock() = default;
        virtual ~Lock() = default;

        virtual void Acquire(State state) = 0;
        virtual void Release(State state) = 0;

        void SetDelegate(const std::weak_ptr<Delegate> &delegate);

    protected:
        std::weak_ptr<Delegate> delegate_;
    };

    static Ptr CreatePlatformDefault();

    SystemController() = default;
    virtual ~SystemController() = default;

    virtual Lock<DisplayState>::Ptr DisplayStateLock() = 0;
};

} // namespace ac

#endif
