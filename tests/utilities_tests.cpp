/*
 * Copyright (C) 2015 Canonical, Ltd.
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

#include <mcs/utils.h>

#include <gtest/gtest.h>

namespace {
const std::string ref{"this is just a test string that helps us to exercise Utilities::StringStartsWith"};
}

TEST(Utilities, StringStartsWithReturnsTrueForSameString)
{
    EXPECT_TRUE(mcs::Utils::StringStartsWith(ref, ref));
}

TEST(Utilities, StringStartsWithReturnsTrueForPrefix)
{
    EXPECT_TRUE(mcs::Utils::StringStartsWith(ref, ref.substr(0, 10)));
}

TEST(Utilities, StringStartsWithReturnsTrueForEmptyString)
{
    EXPECT_TRUE(mcs::Utils::StringStartsWith(ref, ""));
}

TEST(Utilities, StringStartsWithReturnsFalseForPrefixLongerThanString)
{
    EXPECT_FALSE(mcs::Utils::StringStartsWith(ref.substr(0, 10), ref));
}

TEST(Utilities, ParseHexReturnsCorrectValueForHexString)
{
    const int value = 42;
    EXPECT_EQ(value, mcs::Utils::ParseHex(mcs::Utils::Sprintf("%x", value)));
}

TEST(Utilities, ParseHexReturnsThrowsForStringWithout0x)
{
    const int value = 42;
    EXPECT_ANY_THROW(mcs::Utils::ParseHex(mcs::Utils::Sprintf("%x", value).substr(2)));
}

TEST(Utilities, StringSplitReturnsCorrectListOfTokens)
{
    const std::string separated{"a:b:c:d:e:f:g:h:i:j:k:l:m:n"};
    const std::vector<std::string> tokens {
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n"
    };

    EXPECT_EQ(tokens, mcs::Utils::StringSplit(separated, ':'));
}

TEST(Utilities, StringSplitCompressesEmptyTokens)
{
    const std::string separated{"a:b:c:d:e:f:g:h::i:j:k:l:m:n"};
    const std::vector<std::string> tokens {
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n"
    };

    EXPECT_EQ(tokens, mcs::Utils::StringSplit(separated, ':'));
}

TEST(Utilities, StringSplitReturnsEmptyTokens)
{
    const std::string separated{"a:b:c:d:e:f:g:h::i:j:k:l:m:n:"};
    const std::vector<std::string> tokens {
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", ""
    };

    EXPECT_EQ(tokens, mcs::Utils::StringSplit(separated, ':'));
}
