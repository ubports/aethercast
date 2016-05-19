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

#include "ac/utils.h"

#include "ac/report/reportfactory.h"
#include "ac/report/null/nullreportfactory.h"
#include "ac/report/logging/loggingreportfactory.h"
#include "ac/report/lttng/lttngreportfactory.h"

namespace ac {
namespace report {

ReportFactory::Ptr ReportFactory::Create() {
    std::string type = ac::Utils::GetEnvValue("AETHERCAST_REPORT_TYPE");

    if (type == "log")
        return std::make_shared<LoggingReportFactory>();
    else if (type == "lttng")
        return std::make_shared<LttngReportFactory>();

    return std::make_shared<NullReportFactory>();
}

} // namespace report
} // namespace ac
