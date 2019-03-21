/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <outcome/outcome.hpp>
#include <string>

using std::string_literals::operator""s;

#define ILLEGAL_CHAR_MSG "illegal char"s

enum class ConversionErrc {
  Success = 0,      // 0 should not represent an error
  EmptyString = 1,  // (for rationale, see tutorial on error codes)
  IllegalChar = 2,
  TooLong = 3,
};

enum class DivisionErrc {
  DivisionByZero = 1,
};

// clang-format off
OUTCOME_REGISTER_ERROR(ConversionErrc,
  (ConversionErrc::Success, "success")
  (ConversionErrc::EmptyString, "empty string")
  (ConversionErrc::IllegalChar, ILLEGAL_CHAR_MSG)
  (ConversionErrc::TooLong, "too long")
);

OUTCOME_REGISTER_ERROR(DivisionErrc,
  (DivisionErrc::DivisionByZero, "we can't divide by 0")
);
// clang-format on

outcome::result<int> convert(const std::string &str) {
  if (str.empty())
    return ConversionErrc::EmptyString;

  if (!std::all_of(str.begin(), str.end(), ::isdigit))
    return ConversionErrc::IllegalChar;

  if (str.length() > 9)
    return ConversionErrc::TooLong;

  return atoi(str.c_str());
}

outcome::result<int> divide(int a, int b) {
  if (b == 0)
    return DivisionErrc::DivisionByZero;

  return a / b;
}

outcome::result<int> convert_and_divide(const std::string &a,
                                        const std::string &b) {
  OUTCOME_TRY(valA, convert(a));
  OUTCOME_TRY(valB, convert(b));
  OUTCOME_TRY(valDiv, divide(valA, valB));
  return valDiv;
}

/**
 * @given valid arguments for convert_and_divide
 * @when execute method which returns result
 * @then returns value
 */
TEST(Outcome, CorrectCase) {
  if (auto r = convert_and_divide("500", "2")) {
    auto &&val = r.value();
    ASSERT_EQ(val, 250);
  } else {
    FAIL();
  }
}

/**
 * @given arguments to cause conversion error for convert_and_divide
 * @when execute method which returns result
 * @then returns error
 */
TEST(Outcome, ConversionError) {
  if (auto r = convert_and_divide("500", "a")) {
    FAIL();
  } else {
    auto &&err = r.error();
    ASSERT_EQ(err.message(), ILLEGAL_CHAR_MSG);
  }
}

/**
 * @given arguments to cause division error for convert_and_divide
 * @when execute method which returns result
 * @then returns error
 */
TEST(Outcome, DivisionError) {
  if (auto r = convert_and_divide("500", "0")) {
    FAIL();
  } else {
    auto &&err = r.error();
    ASSERT_EQ(std::string{err.category().name()}, "DivisionErrc"s);  // name of the enum
  }
}
