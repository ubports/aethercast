/*
 * Copyright (C) 2014-2016 Canonical Ltd
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

#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/math/special_functions/pow.hpp>

#include <cmath>

#include <iostream>
#include <map>

#include "tests/common/statistics.h"

namespace math = boost::math;

namespace mcs {
namespace testing {

// We are considering the procedure presented in:
//   http://en.wikipedia.org/wiki/Anderson%E2%80%93Darling_test
// section "Test for normality". More specifically, case 4, unknown mean and
// unknown variance.
AndersonDarlingTest::Result
AndersonDarlingTest::for_normality(
        const Sample& result)
{
    double n = result.get_size();
    // Consider the empirical mean & std. dev. here.
    double mean = result.get_mean();
    double std_dev = std::sqrt(result.get_variance());

    std::vector<double> y;

    result.enumerate([&](double value)
    {
        y.push_back((value - mean) / std_dev);
    });

    std::sort(y.begin(), y.end());

    math::normal_distribution<> normal;
    double s = 0.f;

    for (std::size_t i = 1; i <= y.size(); i++)
    {
        auto cdf = math::cdf(normal, y[i-1]);
        s += (2*i-1) * std::log(cdf) + (2*(n-i)+1)*std::log(1-cdf);
    }

    double a2 = -n - 1.f/n * s;

    // We correct for the case that we are considering empirical mean and variance here.
    // See:
    //    Ralph B. D'Agostino (1986). "Tests for the Normal Distribution".
    //    In D'Agostino, R.B. and Stephens, M.A. Goodness-of-Fit Techniques.
    //    New York: Marcel Dekker.
    a2 = a2 * (1 + .75/n - 2.25/math::pow<2>(n));

    return AndersonDarlingTest::Result
    {
        [a2](Confidence alpha)
        {
            static const std::map<Confidence, double> lut
            {
                {Confidence::zero_point_five_percent, 1.159},
                {Confidence::one_percent, 1.035},
                {Confidence::two_point_five_percent, 0.873},
                {Confidence::five_percent, 0.752},
                {Confidence::ten_percent, 0.631}
            };

            return a2 < lut.at(alpha) ? HypothesisStatus::not_rejected : HypothesisStatus::rejected;
        }
    };
}

StudentsTTest::Result StudentsTTest::one_sample(
        const Sample& sample,
        Sample::ValueType mean,
        Sample::ValueType std_dev)
{
    double t = (sample.get_mean() - mean) * std::sqrt(sample.get_size()) / std_dev;
    std::size_t df = sample.get_size() - 1;

    math::students_t_distribution<> dist(df);

    return StudentsTTest::Result
    {
        t,
        [=](double alpha)
        {
            return math::cdf(math::complement(dist, std::fabs(t))) < alpha / 2. ?
                        HypothesisStatus::rejected :
                        HypothesisStatus::not_rejected;
        },
        [=](double alpha)
        {
            return math::cdf(dist, t) < alpha ?
                        HypothesisStatus::not_rejected :
                        HypothesisStatus::rejected;
        },
        [=](double alpha)
        {
            return math::cdf(math::complement(dist, t)) < alpha ?
                        HypothesisStatus::not_rejected :
                        HypothesisStatus::rejected;
        },
    };
}

StudentsTTest::Result StudentsTTest::two_independent_samples(
        const Sample& reference,
        const Sample& sample)
{
    double s1 = reference.get_variance();
    double s2 = sample.get_variance();

    std::size_t n1 = reference.get_size();
    std::size_t n2 = sample.get_size();

    double d = reference.get_mean() - sample.get_mean();

    double s12 = std::sqrt(
                ((n1-1.)*s1 + (n2-1)*s2)/(n1 + n2 - 2));
    double t = d / (s12 * std::sqrt(1./n1 + 1./n2));
    std::size_t df = n1 + n2 - 2;
    math::students_t_distribution<> dist(df);

    return StudentsTTest::Result
    {
        t,
        [=](double alpha)
        {
            return math::cdf(math::complement(dist, std::fabs(t))) < alpha / 2. ?
                        HypothesisStatus::rejected :
                        HypothesisStatus::not_rejected;
        },
        [=](double alpha)
        {
            return math::cdf(dist, t) < alpha ?
                        HypothesisStatus::not_rejected :
                        HypothesisStatus::rejected;
        },
        [=](double alpha)
        {
            return math::cdf(math::complement(dist, t)) < alpha ?
                        HypothesisStatus::not_rejected :
                        HypothesisStatus::rejected;
        },
    };
}

} // namespace testing
} // namespace mcs
