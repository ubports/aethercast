/*
 * Copyright (C) 2014 Canonical Ltd
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

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/archive/archive_exception.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <iostream>

#include "tests/common/benchmark.h"

namespace {
constexpr const char* kNameForMilliseconds{"milliseconds"};
}

namespace boost {
namespace serialization {
template<class Archive>
void load(
        Archive & ar,
        mcs::testing::Benchmark::Result::Timing::Milliseconds& duration,
        const unsigned int)
{
    mcs::testing::Benchmark::Result::Timing::Milliseconds::rep value;
    ar & boost::serialization::make_nvp(kNameForMilliseconds, value);
    duration = mcs::testing::Benchmark::Result::Timing::Milliseconds{value};
}

template<class Archive>
void save(
        Archive & ar,
        const mcs::testing::Benchmark::Result::Timing::Milliseconds& duration,
        const unsigned int)
{
    mcs::testing::Benchmark::Result::Timing::Milliseconds::rep value = duration.count();
    ar & boost::serialization::make_nvp(kNameForMilliseconds, value);
}

template<class Archive>
void serialize(
        Archive & ar,
        mcs::testing::Benchmark::Result::Timing::Milliseconds& duration,
        const unsigned int version)
{
    boost::serialization::split_free(ar, duration, version);
}

template<class Archive>
void serialize(
        Archive & ar,
        std::pair<
            mcs::testing::Benchmark::Result::Timing::Milliseconds,
            double
        >& pair,
        const unsigned int)
{
    ar & boost::serialization::make_nvp("first", pair.first);
    ar & boost::serialization::make_nvp("milliseconds", pair.second);
}

template<class Archive>
void serialize(Archive & ar, mcs::testing::Benchmark::Result& result, const unsigned int)
{
    ar & boost::serialization::make_nvp("sample_size", result.sample_size);
    ar & boost::serialization::make_nvp("timing.min", result.timing.min);
    ar & boost::serialization::make_nvp("timing.max", result.timing.max);
    ar & boost::serialization::make_nvp("timing.mean", result.timing.mean);
    ar & boost::serialization::make_nvp("timing.std_dev", result.timing.std_dev);
    ar & boost::serialization::make_nvp("timing.sample", result.timing.sample);
}
} // namespace boost
} // namespace serialization

namespace mcs {
namespace testing {

void Benchmark::Result::load_from_xml(std::istream& in)
{
    try
    {
        boost::archive::xml_iarchive ia(in);
        ia >> boost::serialization::make_nvp("result", *this);
    } catch(const boost::archive::archive_exception& e)
    {
        throw std::runtime_error(std::string{"Benchmark::Result::load_from_xml: "}+ e.what());
    }
}

void Benchmark::Result::save_to_xml(std::ostream& out)
{
    try
    {
        boost::archive::xml_oarchive oa(out);
        oa << boost::serialization::make_nvp("result", *this);
    } catch(const boost::archive::archive_exception& e)
    {
        throw std::runtime_error(std::string{"Benchmark::Result::save_to_xml: "} + e.what());
    }
}

void Benchmark::Result::load_from(std::istream& in)
{
    try
    {
        boost::archive::text_iarchive ia(in);
        ia >> boost::serialization::make_nvp("result", *this);
    } catch(const boost::archive::archive_exception& e)
    {
        throw std::runtime_error(std::string{"Benchmark::Result::load_from: "}+ e.what());
    }
}

void Benchmark::Result::save_to(std::ostream& out)
{
    try
    {
        boost::archive::text_oarchive oa(out);
        oa << boost::serialization::make_nvp("result", *this);
    } catch(const boost::archive::archive_exception& e)
    {
        throw std::runtime_error(std::string{"Benchmark::Result::save_to: "} + e.what());
    }
}

Sample::SizeType Benchmark::Result::Timing::get_size() const
{
   return sample.size();
}

Sample::ValueType Benchmark::Result::Timing::get_mean() const
{
   return mean.count();
}

Sample::ValueType Benchmark::Result::Timing::get_variance() const
{
   return std_dev.count() * std_dev.count();
}

void Benchmark::Result::Timing::enumerate(const Sample::Enumerator& enumerator) const
{
   for (const auto& observation : sample)
       enumerator(observation.count());
}

bool Benchmark::Result::Timing::is_significantly_faster_than_reference(
        const Benchmark::Result::Timing& reference,
        double alpha) const
{
    if (HypothesisStatus::rejected ==
            AndersonDarlingTest()
            .for_normality(*this)
            .data_fits_normal_distribution(
                Confidence::zero_point_five_percent))
        throw std::runtime_error{"Timing sample is not normally distributed."};

    if (HypothesisStatus::rejected ==
            AndersonDarlingTest()
            .for_normality(reference)
            .data_fits_normal_distribution(
                Confidence::zero_point_five_percent))
        throw std::runtime_error{"Reference timing sample is not normally distributed."};

    auto test_result = StudentsTTest()
            .two_independent_samples(
                reference,
                *this);

    return HypothesisStatus::not_rejected ==
            test_result.sample1_mean_gt_sample2_mean(alpha);
}

bool Benchmark::Result::Timing::is_significantly_faster_than_reference(
        double mean,
        double std_dev,
        double alpha) const
{
    if (HypothesisStatus::rejected ==
            AndersonDarlingTest()
            .for_normality(*this)
            .data_fits_normal_distribution(
                Confidence::zero_point_five_percent))
        throw std::runtime_error{"Timing sample is not normally distributed."};

    auto test_result = StudentsTTest()
            .one_sample(*this, mean, std_dev);

    return HypothesisStatus::not_rejected ==
            test_result.sample1_mean_lt_sample2_mean(alpha);
}

bool Benchmark::Result::Timing::is_significantly_slower_than_reference(
        const Benchmark::Result::Timing& reference,
        double alpha) const
{
    if (HypothesisStatus::rejected ==
            AndersonDarlingTest()
            .for_normality(*this)
            .data_fits_normal_distribution(
                Confidence::zero_point_five_percent))
        throw std::runtime_error{"Timing sample is not normally distributed."};

    if (HypothesisStatus::rejected ==
            AndersonDarlingTest()
            .for_normality(reference)
            .data_fits_normal_distribution(
                Confidence::zero_point_five_percent))
        throw std::runtime_error{"Reference timing sample is not normally distributed."};

    auto test_result = StudentsTTest()
            .two_independent_samples(
                reference,
                *this);

    return HypothesisStatus::not_rejected ==
            test_result.sample1_mean_lt_sample2_mean(alpha);
}

bool Benchmark::Result::Timing::is_significantly_slower_than_reference(
        double mean,
        double std_dev,
        double alpha) const
{
    if (HypothesisStatus::rejected ==
            AndersonDarlingTest()
            .for_normality(*this)
            .data_fits_normal_distribution(
                Confidence::zero_point_five_percent))
        throw std::runtime_error{"Timing sample is not normally distributed."};

    auto test_result = StudentsTTest()
            .one_sample(*this, mean, std_dev);

    return HypothesisStatus::not_rejected ==
            test_result.sample1_mean_gt_sample2_mean(alpha);
}

bool operator==(const Benchmark::Result& lhs, const Benchmark::Result& rhs)
{
    return lhs.sample_size == rhs.sample_size &&
            lhs.timing.mean == rhs.timing.mean &&
            lhs.timing.std_dev == rhs.timing.std_dev &&
            lhs.timing.sample == rhs.timing.sample;
}

std::ostream& operator<<(std::ostream& out, const Benchmark::Result& result)
{
    out << "{"
        << "sample_size: " << result.sample_size << ", "
        << "timing: {"
        << "µ: " << result.timing.mean.count() << " [µs], "
        << "σ: " << result.timing.std_dev.count() << " [µs]"
        << "}}";

    return out;
}

} // namespace testing
} // namespace mcs
