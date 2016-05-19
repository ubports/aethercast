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

#include "ac/report/logging/loggingreportfactory.h"
#include "ac/report/logging/encoderreport.h"
#include "ac/report/logging/rendererreport.h"
#include "ac/report/logging/packetizerreport.h"
#include "ac/report/logging/senderreport.h"

namespace ac {
namespace report {

std::shared_ptr<video::EncoderReport> LoggingReportFactory::CreateEncoderReport() {
    return std::make_shared<logging::EncoderReport>();
}

std::shared_ptr<video::RendererReport> LoggingReportFactory::CreateRendererReport() {
    return std::make_shared<logging::RendererReport>();
}

std::shared_ptr<video::PacketizerReport> LoggingReportFactory::CreatePacketizerReport() {
    return std::make_shared<logging::PacketizerReport>();
}

std::shared_ptr<video::SenderReport> LoggingReportFactory::CreateSenderReport() {
    return std::make_shared<logging::SenderReport>();
}

} // namespace report
} // namespace ac
