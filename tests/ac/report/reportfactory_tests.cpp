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

#include <memory>

#include <gtest/gtest.h>

#include "ac/report/reportfactory.h"
#include "ac/report/null/nullreportfactory.h"
#include "ac/report/logging/loggingreportfactory.h"

using namespace ::testing;

struct ReportFactoryFixture : public ::testing::Test {
    template <typename T>
    void ExceptCorrectType(const std::string &type_name = "", bool set_env = true) {
        if (set_env)
            setenv("AETHERCAST_REPORT_TYPE", type_name.c_str(), 1);
        const auto factory = ac::report::ReportFactory::Create();
        EXPECT_TRUE((dynamic_cast<T*>(factory.get()) ? true : false));
    }
};

TEST_F(ReportFactoryFixture, CreatesCorrectImplementations) {
    ExceptCorrectType<ac::report::NullReportFactory>();
    ExceptCorrectType<ac::report::NullReportFactory>("", false);
    ExceptCorrectType<ac::report::LoggingReportFactory>("log");
}

TEST_F(ReportFactoryFixture, InvalidTypesGiveNull) {
    ExceptCorrectType<ac::report::NullReportFactory>("lalalal");
    ExceptCorrectType<ac::report::NullReportFactory>("12343asd123");
}
