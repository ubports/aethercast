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

#include "mcs/report/reportfactory.h"
#include "mcs/report/null/nullreportfactory.h"
#include "mcs/report/logging/loggingreportfactory.h"

using namespace ::testing;

struct ReportFactoryFixture : public ::testing::Test {
    template <typename T>
    void ExceptCorrectType(const std::string &type_name = "", bool set_env = true) {
        if (set_env)
            setenv("AETHERCAST_REPORT_TYPE", type_name.c_str(), 1);
        const auto factory = mcs::report::ReportFactory::Create();
        EXPECT_TRUE((dynamic_cast<T*>(factory.get()) ? true : false));
    }
};

TEST_F(ReportFactoryFixture, CreatesCorrectImplementations) {
    ExceptCorrectType<mcs::report::NullReportFactory>();
    ExceptCorrectType<mcs::report::NullReportFactory>("", false);
    ExceptCorrectType<mcs::report::LoggingReportFactory>("log");
}

TEST_F(ReportFactoryFixture, InvalidTypesGiveNull) {
    ExceptCorrectType<mcs::report::NullReportFactory>("lalalal");
    ExceptCorrectType<mcs::report::NullReportFactory>("12343asd123");
}
