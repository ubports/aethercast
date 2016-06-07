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

#ifndef AC_REPORT_NULLREPORTFACTORY_H_
#define AC_REPORT_NULLREPORTFACTORY_H_

#include <memory>

#include "ac/non_copyable.h"

#include "ac/report/reportfactory.h"

namespace ac {
namespace report {

class NullReportFactory : public ReportFactory {
public:
    std::shared_ptr<video::EncoderReport> CreateEncoderReport();
    std::shared_ptr<video::RendererReport> CreateRendererReport();
    std::shared_ptr<video::PacketizerReport> CreatePacketizerReport();
    std::shared_ptr<video::SenderReport> CreateSenderReport();
};

} // namespace report
} // namespace ac

#endif
