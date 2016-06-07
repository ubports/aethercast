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

#include "ac/report/lttng/lttngreportfactory.h"
#include "ac/report/lttng/encoderreport.h"
#include "ac/report/lttng/rendererreport.h"
#include "ac/report/lttng/packetizerreport.h"
#include "ac/report/lttng/senderreport.h"

namespace ac {
namespace report {

std::shared_ptr<video::EncoderReport> LttngReportFactory::CreateEncoderReport() {
    return std::make_shared<lttng::EncoderReport>();
}

std::shared_ptr<video::RendererReport> LttngReportFactory::CreateRendererReport() {
    return std::make_shared<lttng::RendererReport>();
}

std::shared_ptr<video::PacketizerReport> LttngReportFactory::CreatePacketizerReport() {
    return std::make_shared<lttng::PacketizerReport>();
}

std::shared_ptr<video::SenderReport> LttngReportFactory::CreateSenderReport() {
    return std::make_shared<lttng::SenderReport>();
}

} // namespace report
} // namespace ac
