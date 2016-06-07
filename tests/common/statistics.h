/*
 * Copyright (C) 2013-2016 Canonical Ltd
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

#ifndef AC_TESTING_COMMON_STATISTICS_H_
#define AC_TESTING_COMMON_STATISTICS_H_

#include <functional>

namespace ac {
namespace testing {

/**
 * \brief The Sample class models the interface to a sample of raw observations
 * and their statistical properties.
 */
class Sample
{
public:
    /** Unsigned type describing the size of the sample. */
    typedef std::size_t SizeType;
    /** Floating point type describing an individual observation. */
    typedef double ValueType;
    /** Function signature for enumerating all raw observations. */
    typedef std::function<void(ValueType)> Enumerator;

    /** \cond */
    virtual ~Sample() = default;

    Sample& operator=(const Sample&) = delete;
    Sample& operator=(Sample&&) = delete;
    bool operator==(const Sample&) const = delete;
    /** \endcond */

    /** Query the size of the sample. */
    virtual SizeType get_size() const = 0;

    /** Query the empirical mean of the sample. */
    virtual ValueType get_mean() const = 0;

    /** Query the empirical variance of the sample. */
    virtual ValueType get_variance() const = 0;

    /** Enumerate all raw observations from the sample. */
    virtual void enumerate(const Enumerator& enumerator) const = 0;

protected:
    /** \cond */
    Sample() = default;
    Sample(const Sample&) = default;
    Sample(Sample&&) = default;
    /** \endcond */
};

/**
 * \brief Summarizes the different outcomes of evaluating a hypothesis at a
 *  given confidence level.
 *
 * Please note that in statistical testing, !(rejected) !=  not_rejected. That is,
 * whenever a hypothesis is rejected, further investigations are required and the
 * conclusion that a hypothesis does not hold is not valid.
 *
 */
enum class HypothesisStatus
{
    rejected, ///< The hypothesis holds at the given confidence level
    not_rejected ///< The hypothesis does not hold at the given confidence level
};

/** \brief We model a hypothesis as a function that can be evaluated for a given confidence value. */
typedef std::function<HypothesisStatus(double)> Hypothesis;

/**
 * \brief The Confidence enum contains well-known confidence levels and is
 * used in cases where only tabulated critical values are available.
 */
enum class Confidence
{
    zero_point_five_percent, ///< alpha = 0.5%
    one_percent, ///< alpha = 1%
    two_point_five_percent, ///< alpha = 2.5%
    five_percent, ///< alpha = 5%
    ten_percent ///< alpha = 10%
};

/** \brief Specialized hypothesis that can only be evaluated at well-known confidence levels. */
typedef std::function<HypothesisStatus(Confidence)> HypothesisForWellKnownConfidence;

/** \brief Implements the Anderson-Darling test for normality for the case of empirical mean and variance.*/
struct AndersonDarlingTest
{
    /**
     * \brief Executing the test returns a set of hypothesis that have to be evaluated
     * at the desired confidence level.
     */
    struct Result
    {
        HypothesisForWellKnownConfidence data_fits_normal_distribution; ///< H0, data is normally distributed
    };

    /**
     * \brief for_normality evaluates a given sample to check if its underlying distribution is normal.
     * \param sample The sample to check.
     * \return A hypothesis containing the test statistics, can be evaluated at different confidence levels.
     */
    Result for_normality(const Sample& sample);
};

/**
 * \brief Implements different variants of the Student's T-test (see http://en.wikipedia.org/wiki/Student's_t-test)
 *
 * \code
 *
 * ac::testing::OutOfProcessBenchmark benchmark;
 *
 *  ac::testing::Result search_result;
 *  ac::ActionMetadata meta_data{default_locale, default_form_factor};
 *
 *  static const std::size_t sample_size{10};
 *  static const std::chrono::seconds per_trial_timeout{1};
 *
 *  ac::testing::Benchmark::PreviewConfiguration config
 *  {
 *      [search_result, meta_data]() { return std::make_pair(search_result, meta_data); },
 *      {
 *          sample_size,
 *          per_trial_timeout
 *      }
 *  };
 *
 * auto result = benchmark.for_preview(scope, config);
 *
 *  auto test_result = ac::testing::StudentsTTest().one_sample(
 *              reference_preview_performance,
 *              result);
 *
 *  EXPECT_EQ(ac::testing::HypothesisStatus::not_rejected,
 *            test_result.sample_mean_is_eq_to_reference(0.05));
 *  EXPECT_EQ(ac::testing::HypothesisStatus::not_rejected,
 *            test_result.sample_mean_is_ge_than_reference(0.05));
 *  EXPECT_EQ(ac::testing::HypothesisStatus::rejected,
 *            test_result.sample_mean_is_le_than_reference(0.05));
 * \endcode
 *
 */
struct StudentsTTest
{
    /**
     * \brief Executing the test returns a set of hypothesis that have to be evaluated
     * at the desired confidence level.
     */
    struct Result
    {
        double t; ///< The t value of the test.
        Hypothesis both_means_are_equal; ///< H0, both means are equal.
        Hypothesis sample1_mean_lt_sample2_mean; ///< H1, sample1 mean < sample2 mean.
        Hypothesis sample1_mean_gt_sample2_mean; ///< H2, sample1 mean > sample2 mean.
    };

    /**
     * \brief one_sample calculates the Student's T test for one sample and a known mean and std. dev..
     * \param sample Sample of values.
     * \param mean The known mean of the underlying distribution
     * \param std_dev The known std. dev. of the underlying distribution
     * \return
     */
    Result one_sample(
            const Sample& sample,
            Sample::ValueType mean,
            Sample::ValueType std_dev);

    /**
     * \brief two_independent_samples calculates the Student's T test for two samples
     * \param sample1 The first sample
     * \param sample2 The second sample
     * \return An instance of Result.
     */
    Result two_independent_samples(
            const Sample& sample1,
            const Sample& sample2);
};

} // namespace testing
} // namespace ac

#endif
