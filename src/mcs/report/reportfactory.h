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

namespace mcs {
namespace video {
class EncoderReport;
class RendererReport;
class PacketizerReport;
class SenderReport;
} // namespace video

namespace report {

class ReportFactory : public mcs::NonCopyable {
public:
    static std::unique_ptr<ReportFactory> Create();

    virtual std::shared_ptr<video::EncoderReport> CreateEncoderReport() = 0;
    virtual std::shared_ptr<video::RendererReport> CreateRendererReport() = 0;
    virtual std::shared_ptr<video::PacketizerReport> CreatePacketizerReport() = 0;
    virtual std::shared_ptr<video::SenderReport> CreateSenderReport() = 0;
};

} // namespace report
} // namespace mcs

#endif
