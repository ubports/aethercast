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

#ifndef MCS_REPORT_REPORTFACTORY_H_
#define MCS_REPORT_REPORTFACTORY_H_

#include <memory>

#include "mcs/non_copyable.h"

#include "mcs/video/encoderreport.h"
#include "mcs/video/rendererreport.h"
#include "mcs/video/packetizerreport.h"
#include "mcs/video/senderreport.h"

namespace mcs {
namespace report {

class ReportFactory : public mcs::NonCopyable {
public:
    static std::unique_ptr<ReportFactory> Create();

    virtual video::EncoderReport::Ptr CreateEncoderReport() = 0;
    virtual video::RendererReport::Ptr CreateRendererReport() = 0;
    virtual video::PacketizerReport::Ptr CreatePacketizerReport() = 0;
    virtual video::SenderReport::Ptr CreateSenderReport() = 0;
};

} // namespace report
} // namespace mcs

#endif
