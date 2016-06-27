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

#include <gmock/gmock.h>

#include <thread>
#include <chrono>

#include <fstream>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/density.hpp>
#include <boost/accumulators/statistics/kurtosis.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/skewness.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>

#include <boost/filesystem.hpp>

#include "ac/mediamanagerfactory.h"
#include "ac/networkutils.h"
#include "ac/utils.h"
#include "ac/logger.h"

#include "ac/systemcontroller.h"

#include "tests/common/benchmark.h"
#include "tests/common/statistics.h"
#include "tests/common/glibhelpers.h"

#include "tests/ac/integration_tests/config.h"

namespace ba = boost::accumulators;
namespace fs = boost::filesystem;

using namespace ::testing;

namespace {
static constexpr unsigned int kStreamMaxUnitSize = 1472;
static constexpr const char *kNullIpAddress{"0.0.0.0"};

class MockStream : public ac::network::Stream {
public:
    MOCK_METHOD2(Connect, bool(const std::string &address, const ac::network::Port &port));
    MOCK_METHOD3(Write, ac::network::Stream::Error(const uint8_t*, unsigned int, const ac::TimestampUs&));
    MOCK_CONST_METHOD0(LocalPort, ac::network::Port());
    MOCK_CONST_METHOD0(MaxUnitSize, std::uint32_t());
};

typedef std::chrono::high_resolution_clock Clock;
typedef ac::testing::Benchmark::Result::Timing::Seconds Resolution;

typedef ba::accumulator_set<
    Resolution::rep,
    ba::stats<ba::tag::count, ba::tag::min, ba::tag::max, ba::tag::mean, ba::tag::variance>
> Statistics;

void FillResultsFromStatistics(ac::testing::Benchmark::Result& result,
                               const Statistics& stats)
{
    result.sample_size = ba::count(stats);

    result.timing.min = Resolution{static_cast<Resolution::rep>(ba::min(stats))};
    result.timing.max = Resolution{static_cast<Resolution::rep>(ba::max(stats))};
    result.timing.mean = Resolution{static_cast<Resolution::rep>(ba::mean(stats))};
    result.timing.std_dev = Resolution{static_cast<Resolution::rep>(std::sqrt(ba::variance(stats)))};
}

class StreamBenchmark : public ac::testing::Benchmark {
public:
    struct PlaybackConfiguration {
        std::chrono::seconds duration{10};
        StatisticsConfiguration statistics_configuration{};
    };

    ac::testing::Benchmark::Result ForPlayback(const PlaybackConfiguration &config) {
        Statistics stats;
        ac::testing::Benchmark::Result benchmark_result;

        auto system_controller = ac::SystemController::CreatePlatformDefault();

        // Make sure the screen is turned on to get the fully boosted
        // system which compares best to a running service.
        system_controller->DisplayStateLock()->Acquire(ac::DisplayState::On);

        const auto output_stream = std::make_shared<MockStream>();

        EXPECT_CALL(*output_stream, MaxUnitSize())
            .WillRepeatedly(Return(kStreamMaxUnitSize));

        EXPECT_CALL(*output_stream, LocalPort())
            .WillRepeatedly(Return(1234));

        EXPECT_CALL(*output_stream, Connect(_, _))
            .WillRepeatedly(Return(true));

        EXPECT_CALL(*output_stream, Write(_, _, _))
            .WillRepeatedly(Invoke([&](const uint8_t *data, unsigned int size, const ac::TimestampUs &timestamp) {
                boost::ignore_unused_variable_warning(data);
                boost::ignore_unused_variable_warning(size);

                const ac::TimestampUs now = ac::Utils::GetNowUs();

                // FIXME there is atleast one buffer which doesn't have a timestamp
                // set. Most propably its the one carrying the CSD data.
                if (timestamp <= 0ll)
                    return ac::network::Stream::Error::kNone;

                const ac::TimestampUs diff = now - timestamp;

                double seconds = diff;
                // Converting from microseconds to seconds as that is what the
                // sample stores internally.
                seconds /= 1000000.0;

                stats(seconds);
                benchmark_result.timing.sample.push_back(ac::testing::Benchmark::Result::Timing::Seconds{seconds});

                return ac::network::Stream::Error::kNone;
            }));

        const auto media_manager = ac::MediaManagerFactory::CreateSource(kNullIpAddress, output_stream);
        const auto port = ac::NetworkUtils::PickRandomPort();

        std::vector<wds::H264VideoCodec> sink_codecs;
        wds::NativeVideoFormat sink_native_format;

        wds::RateAndResolutionsBitmap cea_rr;
        wds::RateAndResolutionsBitmap vesa_rr;
        wds::RateAndResolutionsBitmap hh_rr;

        cea_rr.set(wds::CEA1280x720p30);

        wds::H264VideoCodec codec1(wds::CBP, wds::k3_2, cea_rr, vesa_rr, hh_rr);
        sink_codecs.push_back(codec1);
        wds::H264VideoCodec codec2(wds::CHP, wds::k3_2, cea_rr, vesa_rr, hh_rr);
        sink_codecs.push_back(codec2);

        sink_native_format.type = wds::CEA;
        sink_native_format.rate_resolution = wds::CEA1280x720p30;

        media_manager->SetSinkRtpPorts(port, 0);

        media_manager->InitOptimalVideoFormat(sink_native_format, sink_codecs);
        media_manager->Play();

        // Need to run glib mainloop for a bit to
        ac::testing::RunMainLoop(std::chrono::seconds{1});

        std::this_thread::sleep_for(config.duration);

        media_manager->Teardown();

        system_controller->DisplayStateLock()->Release(ac::DisplayState::Off);

        FillResultsFromStatistics(benchmark_result, stats);
        return benchmark_result;
    }
};
}

TEST(StreamPerformance, EndToEndIsAcceptable) {
    ac::testing::Benchmark::Result reference_result;

    auto ref_path = ac::Utils::GetEnvValue("AETHERCAST_TESTS_REFERENCE_RESULTS",
                                           ac::testing::stream_performance::kReferenceResultFile);
    std::ifstream in{ref_path};
    reference_result.load_from_xml(in);

    StreamBenchmark benchmark;
    const StreamBenchmark::PlaybackConfiguration config;
    auto result = benchmark.ForPlayback(config);

    // We store the potential new reference such that we can copy over if
    // the last test fails.
    std::ofstream out{"ref-new.xml"};
    result.save_to_xml(out);

    AC_DEBUG("current sample size %d mean %f var %f std dev %f",
              result.timing.get_size(), result.timing.get_mean(),
              result.timing.get_variance(), result.timing.get_stddev());
    AC_DEBUG("reference sample size %d  mean %f var %f std dev %f",
              reference_result.timing.get_size(), reference_result.timing.get_mean(),
              reference_result.timing.get_variance(), reference_result.timing.get_stddev());

    // We're only interested in the question if we got slower or not. If
    // we got faster will just produce a warning but will not fail the
    // test. We can't safely error when a test is much faster as this
    // depends very much on what is actually streamed, how long the encoder
    // etc. takes to process.
    ASSERT_FALSE(result.timing.is_significantly_slower_than_reference(reference_result.timing));

    if (result.timing.is_significantly_faster_than_reference(reference_result.timing))
        AC_WARNING("Benchmark shows significantly better performance than reference sample");
}
