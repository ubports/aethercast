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

#include "mcs/report/null/nullreportfactory.h"
#include "mcs/report/null/encoderreport.h"
#include "mcs/report/null/rendererreport.h"
#include "mcs/report/null/packetizerreport.h"
#include "mcs/report/null/senderreport.h"

namespace mcs {
namespace report {

std::shared_ptr<video::EncoderReport> NullReportFactory::CreateEncoderReport() {
    return std::make_shared<null::EncoderReport>();
}

std::shared_ptr<video::RendererReport> NullReportFactory::CreateRendererReport() {
    return std::make_shared<null::RendererReport>();
}

std::shared_ptr<video::PacketizerReport> NullReportFactory::CreatePacketizerReport() {
    return std::make_shared<null::PacketizerReport>();
}

std::shared_ptr<video::SenderReport> NullReportFactory::CreateSenderReport() {
    return std::make_shared<null::SenderReport>();
}

} // namespace report
} // namespace mcs
