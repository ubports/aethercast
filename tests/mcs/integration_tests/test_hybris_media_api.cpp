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

#include <hybris/media/media_codec_source_layer.h>

namespace {
static constexpr const char *kFormatKeyMime{"mime"};
static constexpr const char *kH264MimeType{"video/avc"};
}

TEST(HybrisMedia, DISABLED_SupportsRequiredCodecs) {
    auto format = media_message_create();
    ASSERT_NE(nullptr, format);

    media_message_set_string(format, kFormatKeyMime, kH264MimeType, 0);

    // If this returns a nullptr the encoder creation for the requested
    // format failed and the format isn't supported.
    auto encoder = media_codec_source_create(format, nullptr, 0);
    ASSERT_NE(nullptr, encoder);

    media_message_release(format);
}
