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

#ifndef AC_REPORT_REPORTFACTORY_H_
#define AC_REPORT_REPORTFACTORY_H_

#include <memory>

#include "ac/non_copyable.h"

#include "ac/video/encoderreport.h"
#include "ac/video/rendererreport.h"
#include "ac/video/packetizerreport.h"
#include "ac/video/senderreport.h"

namespace ac {
namespace report {

class ReportFactory : public ac::NonCopyable {
public:
    typedef std::shared_ptr<ReportFactory> Ptr;

    static Ptr Create();

    virtual video::EncoderReport::Ptr CreateEncoderReport() = 0;
    virtual video::RendererReport::Ptr CreateRendererReport() = 0;
    virtual video::PacketizerReport::Ptr CreatePacketizerReport() = 0;
    virtual video::SenderReport::Ptr CreateSenderReport() = 0;
};

} // namespace report
} // namespace ac

#endif
