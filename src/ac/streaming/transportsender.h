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

#ifndef AC_STREAMING_TRANSPORTSENDER_H_
#define AC_STREAMING_TRANSPORTSENDER_H_

#include "ac/non_copyable.h"

#include "ac/video/buffer.h"

namespace ac {
namespace streaming {

class TransportSender : public ac::NonCopyable {
public:
    typedef std::shared_ptr<TransportSender> Ptr;

    class Delegate : public ac::NonCopyable {
    public:
        virtual void OnTransportNetworkError() = 0;
    };

    void SetDelegate(const std::weak_ptr<Delegate> &delegate);
    void ResetDelegate();

    virtual bool Queue(const ac::video::Buffer::Ptr &packets) = 0;
    virtual int32_t LocalPort() const = 0;

protected:
    std::weak_ptr<Delegate> delegate_;
};

} // namespace streaming
} // namespace ac

#endif
