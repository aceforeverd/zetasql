//
// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "zetasql/common/utf_util.h"

#include <cstdint>

#include "zetasql/compliance/functions_testlib_common.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"

namespace zetasql {

static void TestWellFormedString(absl::string_view str) {
  EXPECT_EQ(SpanWellFormedUTF8(str), str.length());
  EXPECT_TRUE(IsWellFormedUTF8(str));
  EXPECT_EQ(CoerceToWellFormedUTF8(str), str);
}

static void TestIllFormedString(absl::string_view str,
                                int expected_error_offset = 0) {
  EXPECT_EQ(SpanWellFormedUTF8(str), expected_error_offset)
      << "str='" << str << "'";
  EXPECT_FALSE(IsWellFormedUTF8(str)) << "str='" << str << "'";
  EXPECT_NE(CoerceToWellFormedUTF8(str), str) << "str='" << str << "'";
}

TEST(UtfUtilTest, WellFormedUTF8) {
  TestWellFormedString("");
  TestWellFormedString("\n");
  TestWellFormedString("\uFFFD");  // REPLACEMENT CHARACTER
  TestWellFormedString("abc");
  TestWellFormedString("abc\u000BBabc");
  TestWellFormedString("\xc2\xbf");
  TestWellFormedString("\xe8\xb0\xb7\xe6\xad\x8c");

  TestIllFormedString("\xc1");
  TestIllFormedString("\xca");
  TestIllFormedString("\xcc");
  TestIllFormedString("\xFA");
  TestIllFormedString("\xc1\xca\x1b\x62\x19o\xcc\x04");

  TestIllFormedString("\xc2\xc0");  // First byte ok UTF-8, invalid together.

  // These are all valid prefixes for UTF-8 characters, but the characters
  // are not complete.
  TestIllFormedString("\xc2");          // Should be 2 byte UTF-8 character.
  TestIllFormedString("\xc3");          // Should be 2 byte UTF-8 character.
  TestIllFormedString("\xe0");          // Should be 3 byte UTF-8 character.
  TestIllFormedString("\xe0\xac");      // Should be 3 byte UTF-8 character.
  TestIllFormedString("\xf0");          // Should be 4 byte UTF-8 character.
  TestIllFormedString("\xf0\x90");      // Should be 4 byte UTF-8 character.
  TestIllFormedString("\xf0\x90\x80");  // Should be 4 byte UTF-8 character.
  TestIllFormedString("A\xf0\x90", 1);
  TestIllFormedString("AB\xf0\x90", 2);
  TestIllFormedString("ABC\xf0\x90", 3);
}

void TestCoerce(std::string str, std::string expected) {
  if (str == expected) {
    // Sanity check.
    EXPECT_TRUE(IsWellFormedUTF8(str));
  }
  EXPECT_EQ(CoerceToWellFormedUTF8(str), expected) << "str: " << str;
}

TEST(UtfUtilTest, CoerceToWellFormedUTF8) {
  TestCoerce("\xc1", "\uFFFD");
  TestCoerce("\uFFFD", "\uFFFD");  // REPLACEMENT CHARACTER is okay.
  TestCoerce("\xc1 ", "\uFFFD ");
  TestCoerce(" \xc1 ", " \uFFFD ");
  TestCoerce("\xc1  \xc1", "\uFFFD  \uFFFD");
  TestCoerce("  \xc1  \xc1  ", "  \uFFFD  \uFFFD  ");

  TestCoerce("\xc1\xca\x1b\x62\x19o\xcc\x04",
             "\uFFFD\uFFFD\x1b\x62\x19o\uFFFD\x04");

  // First byte ok UTF-8, invalid together.
  TestCoerce("\xc2\xc0", "\uFFFD\uFFFD");

  // These are all valid prefixes for UTF-8 characters, but the characters
  // are not complete.
  TestCoerce("\xc2", "\uFFFD");  // Should be 2 byte UTF-8 character.
  TestCoerce("\xc3", "\uFFFD");  // Should be 2 byte UTF-8 character.
  TestCoerce("\xe0", "\uFFFD");  // Should be 3 byte UTF-8 character.
  // Should be 3 byte UTF-8 character.
  TestCoerce("\xe0\xac", "\uFFFD");
  // Should be 4 byte UTF-8 character.
  TestCoerce("\xf0", "\uFFFD");
  // Should be 4 byte UTF-8 character.
  TestCoerce("\xf0\x90", "\uFFFD");
  // Should be 4 byte UTF-8 character.
  TestCoerce("\xf0\x90\x80", "\uFFFD");
  // Should be 2 x 4 byte UTF-8 character.
  TestCoerce("\xf0\x90\x80\xf0\x90\x80", "\uFFFD\uFFFD");
  TestCoerce("\xf0\x90\x80\x80\xf0\x90\x80\xf0\x90\x80\xf0\x90\x80\x80",
             "\xf0\x90\x80\x80\uFFFD\uFFFD\xf0\x90\x80\x80");
  TestCoerce("A\xf0\x90", "A\uFFFD");
  TestCoerce("AB\xf0\x90", "AB\uFFFD");
  TestCoerce("ABC\xf0\x90", "ABC\uFFFD");
}

TEST(UtfUtilTest, PrettyTruncateUTF8) {
  // 10 ascii characters
  std::string str1 = "1234567890";
  EXPECT_EQ(10, str1.size());

  // No truncation.
  EXPECT_EQ(PrettyTruncateUTF8(str1, 11), str1);
  EXPECT_EQ(PrettyTruncateUTF8(str1, 10), str1);

  // Truncate, add ...
  EXPECT_EQ(PrettyTruncateUTF8(str1, 9), "123456...");
  EXPECT_EQ(PrettyTruncateUTF8(str1, 4), "1...");

  // Corner case, drop the ..., pure truncate.
  EXPECT_EQ(PrettyTruncateUTF8(str1, 3), "123");
  EXPECT_EQ(PrettyTruncateUTF8(str1, 2), "12");

  // Be error tolerant, worse case, return empty string.
  EXPECT_EQ(PrettyTruncateUTF8(str1, 0), "");
  EXPECT_EQ(PrettyTruncateUTF8(str1, -4500), "");

  // 10 characters of utf garbage data.
  // Note, this gets exactly the same as ascii.
  std::string str2 = "\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80";
  EXPECT_EQ(10, str2.size());

  // No truncation.
  EXPECT_EQ(PrettyTruncateUTF8(str2, 11), str2);
  EXPECT_EQ(PrettyTruncateUTF8(str2, 10), str2);

  // Truncate, add ...
  EXPECT_EQ(PrettyTruncateUTF8(str2, 9), "\x80\x80\x80\x80\x80\x80...");
  EXPECT_EQ(PrettyTruncateUTF8(str2, 4), "\x80...");

  // Corner case, drop the ..., pure truncate.
  EXPECT_EQ(PrettyTruncateUTF8(str2, 3), "\x80\x80\x80");
  EXPECT_EQ(PrettyTruncateUTF8(str2, 2), "\x80\x80");

  // Be error tolerant, worse case, return empty string.
  EXPECT_EQ(PrettyTruncateUTF8(str2, 0), "");
  EXPECT_EQ(PrettyTruncateUTF8(str2, -4500), "");

  // scattered instances of 2-byte code points, note:
  // ð = "\xc3\xb0"
  // ñ = "\xc3\xb1"
  // ò = "\xc3\xb2"
  std::string str3 = "1ð4ñò90";
  EXPECT_EQ(10, str3.size());

  // No truncation.
  EXPECT_EQ(PrettyTruncateUTF8(str3, 11), str3);
  EXPECT_EQ(PrettyTruncateUTF8(str3, 10), str3);

  // Truncates cleanly between ñ and ò.
  EXPECT_EQ(PrettyTruncateUTF8(str3, 9), "1ð4ñ...");
  // But this slices ñ in half, so it gets dropped completely
  EXPECT_EQ(PrettyTruncateUTF8(str3, 8), "1ð4...");
  // Cleanly slice out the ñ.
  EXPECT_EQ(PrettyTruncateUTF8(str3, 7), "1ð4...");
  EXPECT_EQ(PrettyTruncateUTF8(str3, 4), "1...");

  // Corner case, drop the ...,
  EXPECT_EQ(PrettyTruncateUTF8(str3, 3), "1ð");
  // ... but still don't slice a code point in half.
  EXPECT_EQ(PrettyTruncateUTF8(str3, 2), "1");
  EXPECT_EQ(PrettyTruncateUTF8(str3, 1), "1");

  // Be error tolerant, worse case, return empty string.
  EXPECT_EQ(PrettyTruncateUTF8(str3, 0), "");
  EXPECT_EQ(PrettyTruncateUTF8(str3, -4500), "");
}

TEST(UtfUtilTest, CheckAndCastStrLengthAll) {
  {
    int32_t length;
    std::string str("string");
    EXPECT_TRUE(CheckAndCastStrLength(str, &length));
    EXPECT_EQ(length, str.length());
  }
  {
    int32_t length;
    std::string str(int32max - 1, ' ');
    EXPECT_TRUE(CheckAndCastStrLength(str, &length));
  }
  {
    int32_t length;
    std::string str(int32max, ' ');
    EXPECT_TRUE(CheckAndCastStrLength(str, &length));
    EXPECT_EQ(length, str.length());
  }
  {
    int32_t length;
    std::string str(static_cast<int64_t>(int32max) + 1, ' ');
    EXPECT_FALSE(CheckAndCastStrLength(str, &length));
  }
}

TEST(UtfUtilTest, ForwardNAll) {
  {
    absl::string_view str = "abcd";
    // Move forward 0 code points.
    auto offset = ForwardN(str, static_cast<int32_t>(str.length()),
                           /*num_code_points=*/0);
    EXPECT_EQ(offset.value(), 0);
    offset = ForwardN(str, static_cast<int32_t>(str.length()),
                      /*num_code_points=*/1);
    EXPECT_EQ(offset.value(), 1);
    str.remove_prefix(offset.value());
    // 'bcd' left.
    offset = ForwardN(str, static_cast<int32_t>(str.length()),
                      /*num_code_points=*/2);
    EXPECT_EQ(offset.value(), 2);
    str.remove_prefix(offset.value());
    // 'd' left. Move past end of string.
    offset = ForwardN(str, static_cast<int32_t>(str.length()),
                      /*num_code_points=*/2);
    EXPECT_EQ(offset.value(), 1);
  }
  {
    // Hex equivalent per character (0x61) (0xed 0x9f 0xbf) (0xd1 0x84).
    absl::string_view str = "a퟿ф";
    auto offset = ForwardN(str, static_cast<int32_t>(str.length()),
                           /*num_code_points=*/1);
    EXPECT_EQ(offset.value(), 1);
    str.remove_prefix(offset.value());
    // '퟿ф' left.
    offset = ForwardN(str, static_cast<int32_t>(str.length()),
                      /*num_code_points=*/1);
    EXPECT_EQ(offset.value(), 3);
    str.remove_prefix(offset.value());
    // 'ф' left.
    offset = ForwardN(str, static_cast<int32_t>(str.length()),
                      /*num_code_points=*/1);
    EXPECT_EQ(offset.value(), 2);
  }
  {
    // Return error on invalid utf-8 data.
    // 2nd byte is invalid.
    absl::string_view str = "\x61\xa7\x65\x71";
    auto offset = ForwardN(str, static_cast<int32_t>(str.length()),
                           /*num_code_points=*/1);
    EXPECT_EQ(offset.value(), 1);
    offset = ForwardN(str, static_cast<int32_t>(str.length()),
                      /*num_code_points=*/2);
    EXPECT_EQ(offset, absl::nullopt);
  }
  {
    absl::string_view str = "abcd";
    // Moving negative number of code points does nothing.
    auto offset = ForwardN(str, static_cast<int32_t>(str.length()),
                           /*num_code_points=*/-1);
    EXPECT_EQ(offset.value(), 0);
    // Move number of code points larger than string length.
    offset = ForwardN(str, static_cast<int32_t>(str.length()),
                      /*num_code_points=*/str.length() + 1);
    EXPECT_EQ(offset.value(), str.length());
  }
}

}  // namespace zetasql
