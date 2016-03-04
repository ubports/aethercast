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

#ifndef MCS_TEST_ANDROID_MOCKENCODERREPORT_H_
#define MCS_TEST_ANDROID_MOCKENCODERREPORT_H_

#include <gmock/gmock.h>

#include "mcs/video/encoderreport.h"

namespace mcs {
namespace test {
namespace android {

class MockEncoderReport : public video::EncoderReport {
public:
    MOCK_METHOD0(Started, void());
    MOCK_METHOD0(Stopped, void());
    MOCK_METHOD1(BeganFrame, void(const mcs::TimestampUs));
    MOCK_METHOD1(FinishedFrame, void(const mcs::TimestampUs));
    MOCK_METHOD1(ReceivedInputBuffer, void(const mcs::TimestampUs));
};

} // namespace android
} // namespace tests
} // namespace mcs

#endif
