/*
 * Copyright (C) 2013 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MCS_TESTING_COMMON_BENCHMARK_H_
#define MCS_TESTING_COMMON_BENCHMARK_H_

#include <chrono>
#include <functional>
#include <iosfwd>
#include <memory>

#include "tests/common/statistics.h"

namespace mcs {
namespace testing {

/**
 * \brief The Benchmark class defines an interface to provide scope authors with runtime benchmarking capabilities
 * to be used in their own testing.
 */
class Benchmark
{
public:
    /**
     * \brief The Result struct encapsulates all of the result gathered from one
     * individual benchmark run consisting of multiple independent trials.
     */
    struct Result
    {
        /** Size of the sample, corresponds to number of trials. */
        std::size_t sample_size{0};
        /** Timing characteristics captured during the benchmark run. */
        struct Timing : public Sample
        {
            /** All timing-based results are measures in seconds. */
            typedef std::chrono::duration<double> Seconds;

            /** \cond */
            Timing() = default;
            Timing(const Timing&) = default;
            Timing(Timing&&) = default;
            /** \endcond */

             /** Query the size of the sample. */
            Sample::SizeType get_size() const;
            /** Query the empirical mean of the sample. */
            Sample::ValueType get_mean() const;
            /** Query the empirical variance of the sample. */
            Sample::ValueType get_variance() const;
            /** Query the standard devariation of the sample */
            Sample::ValueType get_stddev() const;
            /** Enumerate all raw observations from the sample. */
            void enumerate(const Sample::Enumerator& enumerator) const;

            /**
             * \brief Checks if a timing sample is statistically significantly
             * faster than a reference timing sample.
             * \param reference The reference timing sample to compare to.
             * \param alpha The critical value of the statistical test. The lower, the higher the relevance of the test.
             * \return true iff this timing sample is significantly faster than the reference sample.
             */
            bool is_significantly_faster_than_reference(const Timing& reference, double alpha = 0.05) const;

            /**
             * \brief Checks if a timing sample is statistically significantly
             * faster than a reference timing with mean 'mean and std. dev. 'std_dev'
             * \throw std::runtime_error if this sample is not normally distributed.
             * \param mean The reference mean to compare to.
             * \param std_dev The reference std. dev. to compare to.
             * \param alpha The critical value of the statistical test. The lower, the higher the relevance of the test.
             * \return true iff this timing sample is significantly faster than the reference sample.
             */
            bool is_significantly_faster_than_reference(
                    double mean,
                    double std_dev,
                    double alpha = 0.05) const;

            /**
             * \brief Checks if a timing sample is statistically significantly
             * slower than a reference timing sample.
             * \param reference The reference timing sample to compare to.
             * \param alpha The critical value of the statistical test. The lower, the higher the relevance of the test.
             * \return true iff this timing sample is significantly slower than the reference.
             */
            bool is_significantly_slower_than_reference(const Timing& reference, double alpha = 0.05) const;

            /**
             * \brief Checks if a timing sample is statistically significantly
             * slower than a reference timing with mean 'mean and std. dev. 'std_dev'
             * \param mean The reference mean to compare to.
             * \param std_dev The reference std. dev. to compare to.
             * \param alpha The critical value of the statistical test. The lower, the higher the relevance of the test.
             * \return true iff this timing sample is significantly slower than the reference.
             */
            bool is_significantly_slower_than_reference(
                    double mean,
                    double std_dev,
                    double alpha = 0.05) const;

            /** Minimum execution time for the benchmarked operation. */
            Seconds min{Seconds::min()};
            /** Maximum execution time for the benchmarked operation. */
            Seconds max{Seconds::min()};
            /** Mean execution time for the benchmarked operation. */
            Seconds mean{Seconds::min()};
            /** Std. deviation in execution time for the benchmarked operation. */
            Seconds std_dev{Seconds::min()};
            /** Raw sample vector, with sample.size() == sample_size */
            std::vector<Seconds> sample{};
        } timing{}; ///< Runtime-specific sample data.

        /**
         * \brief load_from restores a result from the given input stream.
         * \throw std::runtime_error in case of issues.
         * \param in The stream to read from.
         */
        void load_from(std::istream& in);

        /**
         * \brief save_to stores a result to the given output stream.
         * \throw std::runtime_error in case of issues.
         * \param out The stream to write to.
         */
        void save_to(std::ostream& out);

        /**
         * \brief load_from_xml restores a result stored as xml from the given input stream.
         * \throw std::runtime_error in case of issues.
         * \param in The stream to read from.
         */
        void load_from_xml(std::istream& in);

        /**
         * \brief save_to_xml stores a result as xml to the given output stream.
         * \throw std::runtime_error in case of issues.
         * \param out The stream to write to.
         */
        void save_to_xml(std::ostream& out);
    };

    /**
     * \brief The StatisticsConfiguration struct contains options controlling
     * the calculation of benchmark result statistics.
     */
    struct StatisticsConfiguration
    {
        /** Number of bins in the final histogram. */
        std::size_t histogram_bin_count{10};
    };

    /**
     * \brief The TrialConfiguration struct contains options controlling
     * the execution of individual trials.
     */
    struct TrialConfiguration
    {
        /** The number of independent trials. Please note that the number should not be << 10 */
        std::size_t trial_count{25};
        /** Wait at most this time for one trial to finish or throw if a timeout is encountered. */
        std::chrono::microseconds per_trial_timeout{std::chrono::seconds{10}};
        /** Fold in statistics configuration into the overall trial setup. */
        StatisticsConfiguration statistics_configuration{};
    };

    /** \cond */
    virtual ~Benchmark() = default;
    Benchmark(const Benchmark&) = delete;
    Benchmark(Benchmark&&) = delete;

    Benchmark& operator=(const Benchmark&) = delete;
    Benchmark& operator=(Benchmark&&) = delete;
    /** \endcond */

protected:
    Benchmark() = default;
};

bool operator==(const Benchmark::Result& lhs, const Benchmark::Result& rhs);

std::ostream& operator<<(std::ostream&, const Benchmark::Result&);

} // namespace testing
} // namespace mcs

#endif
