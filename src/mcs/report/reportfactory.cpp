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

#include "mcs/utils.h"

#include "mcs/report/reportfactory.h"
#include "mcs/report/null/nullreportfactory.h"
#include "mcs/report/logging/loggingreportfactory.h"
#include "mcs/report/lttng/lttngreportfactory.h"

namespace mcs {
namespace report {

std::unique_ptr<ReportFactory> ReportFactory::Create() {
    std::string type = mcs::Utils::GetEnvValue("AETHERCAST_REPORT_TYPE");

    if (type == "log")
        return std::make_unique<LoggingReportFactory>();
    else if (type == "lttng")
        return std::make_unique<LttngReportFactory>();

    return std::make_unique<NullReportFactory>();
}

} // namespace report
} // namespace mcs
