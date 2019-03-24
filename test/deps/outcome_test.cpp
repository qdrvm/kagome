/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <outcome/outcome.hpp>
#include <string>

using std::string_literals::operator""s;

#define ILLEGAL_CHAR_MSG "illegal char"s
#define DIV_0_MSG "division by 0"s

enum class ConversionErrc {
  Success = 0,      // 0 should not represent an error
  EmptyString = 1,  // (for rationale, see tutorial on error codes)
  IllegalChar = 2,
  TooLong = 3,
};

enum class DivisionErrc {
  DivisionByZero = 1,
};

OUTCOME_MAKE_ERROR_CODE(ConversionErrc);
OUTCOME_MAKE_ERROR_CODE(DivisionErrc);

OUTCOME_REGISTER_CATEGORY(ConversionErrc, e) {
  switch (e) {
    case ConversionErrc ::Success:
      return "success";
    case ConversionErrc ::EmptyString:
      return "empty string";
    case ConversionErrc ::IllegalChar:
      return ILLEGAL_CHAR_MSG;
    case ConversionErrc ::TooLong:
      return "too long";
    default:
      return "unknown";
  }
}

OUTCOME_REGISTER_CATEGORY(DivisionErrc, e) {
  switch (e) {
    case DivisionErrc ::DivisionByZero:
      return "division by 0";
    default:
      return "unknown";
  }
}

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
  auto r = convert_and_divide("500", "2");
  ASSERT_TRUE(r);
  auto &&val = r.value();
  ASSERT_EQ(val, 250);
}

/**
 * @given arguments to cause conversion error for convert_and_divide
 * @when execute method which returns result
 * @then returns error
 */
TEST(Outcome, ConversionError) {
  auto r = convert_and_divide("500", "a");
  ASSERT_FALSE(r);
  auto &&err = r.error();
  ASSERT_EQ(err.message(), ILLEGAL_CHAR_MSG);
}

/**
 * @given arguments to cause division error for convert_and_divide
 * @when execute method which returns result
 * @then returns error
 */
TEST(Outcome, DivisionError) {
  auto r = convert_and_divide("500", "0");
  ASSERT_FALSE(r);
  auto &&err = r.error();
  ASSERT_EQ(err.message(), DIV_0_MSG);  // name of the enum
}
