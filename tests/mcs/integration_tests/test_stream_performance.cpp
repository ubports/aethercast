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

#include "mcs/mediamanagerfactory.h"
#include "mcs/networkutils.h"
#include "mcs/utils.h"
#include "mcs/logger.h"

#include "mcs/systemcontroller.h"

#include "tests/common/benchmark.h"
#include "tests/common/statistics.h"

namespace ba = boost::accumulators;

using namespace ::testing;

namespace {
static constexpr unsigned int kStreamMaxUnitSize = 1472;
static constexpr const char *kNullIpAddress{"0.0.0.0"};
static constexpr const char *kReferenceResultFile{"ref.xml"};

class MockStream : public mcs::network::Stream {
public:
    MOCK_METHOD2(Connect, bool(const std::string &address, const mcs::network::Port &port));
    MOCK_METHOD0(WaitUntilReady, bool());
    MOCK_METHOD3(Write, mcs::network::Stream::Error(const uint8_t*, unsigned int, const mcs::TimestampUs&));
    MOCK_CONST_METHOD0(LocalPort, mcs::network::Port());
    MOCK_CONST_METHOD0(MaxUnitSize, std::uint32_t());
};

typedef std::chrono::high_resolution_clock Clock;
typedef mcs::testing::Benchmark::Result::Timing::Seconds Resolution;

typedef ba::accumulator_set<
    Resolution::rep,
    ba::stats<ba::tag::count, ba::tag::min, ba::tag::max, ba::tag::mean, ba::tag::variance>
> Statistics;

void FillResultsFromStatistics(mcs::testing::Benchmark::Result& result,
                               const Statistics& stats)
{
    result.sample_size = ba::count(stats);

    result.timing.min = Resolution{static_cast<Resolution::rep>(ba::min(stats))};
    result.timing.max = Resolution{static_cast<Resolution::rep>(ba::max(stats))};
    result.timing.mean = Resolution{static_cast<Resolution::rep>(ba::mean(stats))};
    result.timing.std_dev = Resolution{static_cast<Resolution::rep>(std::sqrt(ba::variance(stats)))};
}

class StreamBenchmark : public mcs::testing::Benchmark {
public:
    struct PlaybackConfiguration {
        std::chrono::seconds duration{5};
        StatisticsConfiguration statistics_configuration{};
    };

    mcs::testing::Benchmark::Result ForPlayback(const PlaybackConfiguration &config) {
        Statistics stats;
        mcs::testing::Benchmark::Result benchmark_result;

        auto system_controller = mcs::SystemController::CreatePlatformDefault();

        // Make sure the screen is turned on to get the fully boosted
        // system which compares best to a running service.
        system_controller->DisplayStateLock()->Acquire(mcs::DisplayState::On);

        const auto output_stream = std::make_shared<MockStream>();

        EXPECT_CALL(*output_stream, MaxUnitSize())
            .WillRepeatedly(Return(kStreamMaxUnitSize));

        EXPECT_CALL(*output_stream, LocalPort())
            .WillRepeatedly(Return(1234));

        EXPECT_CALL(*output_stream, Connect(_, _))
            .WillRepeatedly(Return(true));

        EXPECT_CALL(*output_stream, WaitUntilReady())
            .WillRepeatedly(Return(true));

        EXPECT_CALL(*output_stream, Write(_, _, _))
            .WillRepeatedly(Invoke([&](const uint8_t *data, unsigned int size, const mcs::TimestampUs &timestamp) {
                boost::ignore_unused_variable_warning(data);
                boost::ignore_unused_variable_warning(size);
                const mcs::TimestampUs now = mcs::Utils::GetNowUs();
                if (timestamp <= 0ll) {
                    MCS_DEBUG("timestamp %lld", timestamp);
                    return mcs::network::Stream::Error::kNone;
                }
                const mcs::TimestampUs diff = now - timestamp;

                double seconds = diff;
                // Converting from microseconds to seconds
                seconds /= (1000000.0);
                if (seconds > 1) {
                    MCS_DEBUG("second > 1 -> %d s (now %lld timestamp %lld)", seconds, now, timestamp);
                    return mcs::network::Stream::Error::kNone;
                }
                stats(seconds);
                benchmark_result.timing.sample.push_back(mcs::testing::Benchmark::Result::Timing::Seconds{seconds});
                return mcs::network::Stream::Error::kNone;
            }));

        const auto media_manager = mcs::MediaManagerFactory::CreateSource(kNullIpAddress, output_stream);
        const auto port = mcs::NetworkUtils::PickRandomPort();

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

        std::this_thread::sleep_for(config.duration);

        media_manager->Teardown();

        system_controller->DisplayStateLock()->Acquire(mcs::DisplayState::Off);

        FillResultsFromStatistics(benchmark_result, stats);
        return benchmark_result;
    }
};
}

TEST(StreamPerformance, EndToEndIsAcceptable) {
    mcs::testing::Benchmark::Result reference_result;

    std::ifstream in{kReferenceResultFile};
    reference_result.load_from_xml(in);

    StreamBenchmark benchmark;
    const StreamBenchmark::PlaybackConfiguration config;
    auto result = benchmark.ForPlayback(config);

    // We store the potential new reference such that we can copy over if
    // the last test fails.
    std::ofstream out{"ref-new.xml"};
    result.save_to_xml(out);

    EXPECT_FALSE(result.timing.is_significantly_faster_than_reference(reference_result.timing));
    EXPECT_FALSE(result.timing.is_significantly_slower_than_reference(reference_result.timing));
}
