/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include <core/testing/fork_and_run.h>

#include <core/posix/signal.h>

#include <gtest/gtest.h>

namespace
{
struct TestingMacrosFixture : public ::testing::Test
{
    TestingMacrosFixture() = default;
};

::testing::AssertionResult ClientFailed(core::testing::ForkAndRunResult result)
{
    return
            (result & core::testing::ForkAndRunResult::client_failed) == core::testing::ForkAndRunResult::empty ?
              ::testing::AssertionFailure() :
              ::testing::AssertionSuccess();
}

::testing::AssertionResult ServiceFailed(core::testing::ForkAndRunResult result)
{
    return
            (result & core::testing::ForkAndRunResult::service_failed) == core::testing::ForkAndRunResult::empty ?
              ::testing::AssertionFailure() :
              ::testing::AssertionSuccess();
}
struct SigTermCatcher
{
    static void sig_term_handler(int)
    {
        std::cout << "Received sigterm." << std::endl;
    }

    SigTermCatcher()
    {
        signal(static_cast<int>(core::posix::Signal::sig_term), sig_term_handler);
    }
} sig_term_catcher;
}

TEST(ForkAndRun, succeeding_client_and_service_result_in_correct_return_value)
{
    auto service = [](){ return core::posix::exit::Status::success; };
    auto client = [](){ return core::posix::exit::Status::success; };

    auto result = core::testing::fork_and_run(service, client);

    ASSERT_FALSE(ClientFailed(result));
    ASSERT_FALSE(ServiceFailed(result));
}

TEST(ForkAndRun, succeeding_client_and_failing_service_result_in_correct_return_value)
{
    auto service = [](){ return core::posix::exit::Status::failure; };
    auto client = [](){ return core::posix::exit::Status::success; };

    auto result = core::testing::fork_and_run(service, client);

    EXPECT_FALSE(ClientFailed(result));
    EXPECT_TRUE(ServiceFailed(result));
}

TEST(ForkAndRun, failing_client_and_failing_service_result_in_correct_return_value)
{
    auto service = [](){ return core::posix::exit::Status::failure; };
    auto client = [](){ return core::posix::exit::Status::failure; };

    auto result = core::testing::fork_and_run(service, client);

    EXPECT_TRUE(ClientFailed(result));
    EXPECT_TRUE(ServiceFailed(result));
}

TEST(ForkAndRun, throwing_client_is_reported_as_failing)
{
    auto service = [](){ return core::posix::exit::Status::success; };
    auto client = [](){ throw std::runtime_error("failing client"); return core::posix::exit::Status::success; };

    auto result = core::testing::fork_and_run(service, client);

    EXPECT_TRUE(ClientFailed(result));
    EXPECT_FALSE(ServiceFailed(result));
}

TEST(ForkAndRun, exiting_with_failure_client_is_reported_as_failing)
{
    auto service = [](){ return core::posix::exit::Status::success; };
    auto client = [](){ exit(EXIT_FAILURE); return core::posix::exit::Status::success; };

    auto result = core::testing::fork_and_run(service, client);

    EXPECT_TRUE(ClientFailed(result));
    EXPECT_FALSE(ServiceFailed(result));
}

TEST(ForkAndRun, aborting_client_is_reported_as_failing)
{
    auto service = [](){ return core::posix::exit::Status::success; };
    auto client = [](){ abort(); return core::posix::exit::Status::success; };

    auto result = core::testing::fork_and_run(service, client);

    EXPECT_TRUE(ClientFailed(result));
    EXPECT_FALSE(ServiceFailed(result));
}

TESTP(TestingMacros, test_fp_macro_reports_success_for_passing_test,
    {
        return core::posix::exit::Status::success;
    })

TESTP_F(TestingMacrosFixture, test_fp_macro_reports_success_for_passing_test,
    {
        return core::posix::exit::Status::success;
    })

// The following two tests fail, and rightly so. However, translating this
// failing behavior to success is really difficult and we omit it for now.
TESTP(TestingMacros, DISABLED_test_fp_macro_reports_success_for_failing_test,
    {
        return core::posix::exit::Status::failure;
    })

TESTP_F(TestingMacrosFixture, DISABLED_test_fp_macro_reports_success_for_failing_test,
    {
        return core::posix::exit::Status::failure;
    })

#include <core/posix/backtrace.h>

TEST(BacktraceSymbolDemangling, demangling_a_cpp_symbol_works)
{
    const char* ref = "tests/fork_and_run_test(_ZN7testing8internal35HandleExceptionsInMethodIfSupportedINS0_12UnitTestImplEbEET0_PT_MS4_FS3_vEPKc+0x4b) [0x4591f8]";
    const char* ref_demangled = "bool testing::internal::HandleExceptionsInMethodIfSupported<testing::internal::UnitTestImpl, bool>(testing::internal::UnitTestImpl*, bool (testing::internal::UnitTestImpl::*)(), char const*)";
    auto symbol = core::posix::backtrace::Frame::Symbol::for_testing_from_raw_symbol(ref);

    EXPECT_TRUE(symbol->is_cxx());
    EXPECT_EQ(ref, symbol->raw());
    EXPECT_EQ(ref_demangled, symbol->demangled());
}
