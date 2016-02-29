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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <mcs/utils.h>

#include <mcs/video/buffer.h>

using namespace mcs::video;

namespace {
class MockBufferDelegate : public Buffer::Delegate {
public:
    MOCK_METHOD1(OnBufferFinished, void(const mcs::video::Buffer::Ptr&));
};
}

TEST(Buffer, ConstructWithCapacity) {
    uint32_t capacity = 10;
    auto now = mcs::Utils::GetNowUs();
    auto buffer = Buffer::Create(capacity, now);

    EXPECT_EQ(now, buffer->Timestamp());
    EXPECT_NE(nullptr, buffer->Data());
    EXPECT_EQ(capacity, buffer->Length());
    EXPECT_EQ(capacity, buffer->Capacity());
    EXPECT_EQ(0, buffer->Data()[0]);
    EXPECT_EQ(0, buffer->Offset());
    EXPECT_TRUE(buffer->IsValid());
}

TEST(Buffer, ConstructWithData) {
    uint8_t test_data[] = { 0xff, 0xee, 0xdd, 0xcc };
    uint32_t test_data_size = 4;

    auto buffer = Buffer::Create(test_data, test_data_size);

    EXPECT_NE(nullptr, buffer->Data());
    EXPECT_EQ(test_data_size, buffer->Length());
    EXPECT_EQ(test_data_size, buffer->Capacity());
    EXPECT_EQ(0, buffer->Offset());
    EXPECT_EQ(0, buffer->Timestamp());
    EXPECT_TRUE(buffer->IsValid());
    // Buffer copies test_data internally so pointers shouldn't match
    EXPECT_NE(test_data, buffer->Data());
    EXPECT_EQ(test_data[0], buffer->Data()[0]);
    EXPECT_EQ(test_data[1], buffer->Data()[1]);
    EXPECT_EQ(test_data[2], buffer->Data()[2]);
    EXPECT_EQ(test_data[3], buffer->Data()[3]);
}

TEST(Buffer, ConstructWithNativeHandle) {
    void *native_handle = reinterpret_cast<void*>(0x11223344);
    auto buffer = Buffer::Create(native_handle);

    EXPECT_EQ(native_handle, buffer->NativeHandle());
    EXPECT_EQ(0, buffer->Length());
    EXPECT_EQ(0, buffer->Capacity());
    EXPECT_EQ(nullptr, buffer->Data());
    EXPECT_EQ(0, buffer->Offset());
    EXPECT_EQ(0, buffer->Timestamp());
    EXPECT_TRUE(buffer->IsValid());
}

TEST(Buffer, DelegateCalledOnRelease) {
    auto buffer = Buffer::Create(nullptr);

    auto delegate = std::make_shared<MockBufferDelegate>();
    EXPECT_CALL(*delegate, OnBufferFinished(buffer)).Times(1);

    buffer->SetDelegate(delegate);
    buffer->Release();
}

TEST(Buffer, RangeSelection) {
    uint8_t test_data[] = { 0xff, 0xee, 0xdd, 0xcc };
    uint32_t test_data_size = 4;

    auto buffer = Buffer::Create(test_data, test_data_size);

    buffer->SetRange(2, 2);
    EXPECT_EQ(test_data[2], buffer->Data()[0]);
    EXPECT_EQ(test_data[3], buffer->Data()[1]);

    buffer->SetRange(1, 1);
    EXPECT_EQ(test_data[1], buffer->Data()[0]);

    buffer->SetRange(-1, -1);
    EXPECT_EQ(test_data[1], buffer->Data()[0]);
}
