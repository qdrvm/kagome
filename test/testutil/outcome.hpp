/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GTEST_OUTCOME_UTIL_HPP
#define KAGOME_GTEST_OUTCOME_UTIL_HPP

#include <gtest/gtest.h>
#include "common/visitor.hpp"
#include "outcome/outcome.hpp"

#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a##b)
#define PP_CAT_II(p, res) res

#define UNIQUE_NAME(base) PP_CAT(base, __LINE__)

#define EXPECT_OUTCOME_TRUE_void(var, expr) \
  auto &&var = expr;                        \
  EXPECT_TRUE(var) << "Line " << __LINE__ << ": " << var.error().message();

#define EXPECT_OUTCOME_TRUE_name(var, val, expr)                            \
  auto &&var = expr;                                                        \
  EXPECT_TRUE(var) << "Line " << __LINE__ << ": " << var.error().message(); \
  auto &&val = var.value();

#define EXPECT_OUTCOME_FALSE_void(var, expr) \
  auto &&var = expr;                         \
  EXPECT_FALSE(var) << "Line " << __LINE__ << ": " << var.error().message();

#define EXPECT_OUTCOME_FALSE_name(var, val, expr)                            \
  auto &&var = expr;                                                         \
  EXPECT_FALSE(var) << "Line " << __LINE__ << ": " << var.error().message(); \
  auto &&val = var.error();

#define EXPECT_OUTCOME_TRUE_3(var, val, expr) \
  EXPECT_OUTCOME_TRUE_name(var, val, expr)

#define EXPECT_OUTCOME_TRUE_2(val, expr) \
  EXPECT_OUTCOME_TRUE_3(UNIQUE_NAME(_r), val, expr)

#define EXPECT_OUTCOME_TRUE_1(expr) \
  EXPECT_OUTCOME_TRUE_void(UNIQUE_NAME(_v), expr)

#define EXPECT_OUTCOME_FALSE_3(var, val, expr) \
  EXPECT_OUTCOME_FALSE_name(var, val, expr)

#define EXPECT_OUTCOME_FALSE_2(val, expr) \
  EXPECT_OUTCOME_FALSE_3(UNIQUE_NAME(_r), val, expr)

#define EXPECT_OUTCOME_FALSE_1(expr) \
  EXPECT_OUTCOME_FALSE_void(UNIQUE_NAME(_v), expr)

/**
 * Use this macro in GTEST with 2 arguments to assert that getResult()
 * returned VALUE and immediately get this value.
 * EXPECT_OUTCOME_TRUE(val, getResult());
 */
#define EXPECT_OUTCOME_TRUE(val, expr) \
  EXPECT_OUTCOME_TRUE_name(UNIQUE_NAME(_r), val, expr)

#define EXPECT_OUTCOME_FALSE(val, expr) \
  EXPECT_OUTCOME_FALSE_name(UNIQUE_NAME(_f), val, expr)

#define EXPECT_OUTCOME_TRUE_MSG_void(var, expr, msg)                       \
  auto &&var = expr;                                                       \
  EXPECT_TRUE(var) << "Line " << __LINE__ << ": " << var.error().message() \
                   << "\t" << (msg);

#define EXPECT_OUTCOME_TRUE_MSG_name(var, val, expr, msg)                  \
  auto &&var = expr;                                                       \
  EXPECT_TRUE(var) << "Line " << __LINE__ << ": " << var.error().message() \
                   << "\t" << (msg);                                       \
  auto &&val = var.value();

/**
 * Use this macro in GTEST with 2 arguments to assert that
 * result of expression as outcome::result<T> is value and,
 * but the value itself is not necessary.
 * If result is error, macro prints corresponding error message
 * and appends custom error message specified in msg.
 */
#define EXPECT_OUTCOME_TRUE_MSG_1(expr, msg) \
  EXPECT_OUTCOME_TRUE_MSG_void(UNIQUE_NAME(_v), expr, msg)

/**
 * Use this macro in GTEST with 3 arguments to assert that
 * result of expression as outcome::result<T> is value and
 * immediately get access to this value.
 * If result is error, macro prints corresponding error message
 * and appends custom error message specified in msg.
 */
#define EXPECT_OUTCOME_TRUE_MSG(val, expr, msg) \
  EXPECT_OUTCOME_TRUE_MSG_name(UNIQUE_NAME(_r), val, expr, msg)

#define TESTUTIL_OUTCOME_GLUE2(x, y) x##y
#define TESTUTIL_OUTCOME_GLUE(x, y) TESTUTIL_OUTCOME_GLUE2(x, y)
#define TESTUTIL_OUTCOME_UNIQUE_NAME \
  TESTUTIL_OUTCOME_GLUE(__outcome_result, __COUNTER__)

#define ASSERT_OUTCOME_SUCCESS_TRY_(result, expr)                     \
  auto &&result = (expr);                                             \
  if (__builtin_expect(result.has_value(), true))                     \
    ;                                                                 \
  else                                                                \
    GTEST_FATAL_FAILURE_("Outcome of: " #expr)                        \
        << "  Actual:   Error '" << result.error().message() << "'\n" \
        << "Expected:   Success";

#define ASSERT_OUTCOME_SUCCESS_(result, val, expr) \
  ASSERT_OUTCOME_SUCCESS_TRY_(result, expr);       \
  auto &&val = std::move(result.value());

#define ASSERT_OUTCOME_FAILURE_(result, expr)                                 \
  {                                                                           \
    auto &&result = (expr);                                                   \
    if (__builtin_expect(result.has_error(), true))                           \
      ;                                                                       \
    else                                                                      \
      GTEST_FATAL_FAILURE_("Outcome of: " #expr) << "  Actual:   Success\n"   \
                                                 << "Expected:   Some error"; \
  }

#define EXPECT_OUTCOME_FAILURE_(result, error, expr)                           \
  auto &&result = (expr);                                                      \
  if (__builtin_expect(result.has_error(), true))                              \
    ;                                                                          \
  else                                                                         \
    GTEST_NONFATAL_FAILURE_("Outcome of: " #expr) << "  Actual:   Success\n"   \
                                                  << "Expected:   Some error"; \
  auto &&error = std::move(result.error());

#define ASSERT_OUTCOME_ERROR_(res, expr, error_id)                          \
  {                                                                         \
    auto &&res = (expr);                                                    \
    if (__builtin_expect(res.has_error(), true)) {                          \
      auto &&expected_error_message =                                       \
          outcome::result<void>(error_id).error().message();                \
      if (__builtin_expect(res.error().message() == expected_error_message, \
                           true)) {                                         \
        ;                                                                   \
      } /* NOLINT */                                                        \
      else                                                                  \
        GTEST_FATAL_FAILURE_("Outcome of: " #expr)                          \
            << "  Actual:   Error '" << res.error().message() << "'\n"      \
            << "Expected:   Error '"                                        \
            << outcome::result<void>(error_id).error().message() << "'";    \
    } else                                                                  \
      GTEST_FATAL_FAILURE_("Outcome of: " #expr)                            \
          << "  Actual:   Success\n"                                        \
          << "Expected:   Error '"                                          \
          << outcome::result<void>(error_id).error().message() << "'";      \
  }

#define EXPECT_OUTCOME_ERROR_(res, error, expr, error_id)                 \
  auto &&res = (expr);                                                    \
  if (__builtin_expect(res.has_error(), true)) {                          \
    auto &&expected_error_message =                                       \
        outcome::result<void>(error_id).error().message();                \
    if (__builtin_expect(res.error().message() == expected_error_message, \
                         true)) {                                         \
      ;                                                                   \
    } /* NOLINT */                                                        \
    else                                                                  \
      GTEST_NONFATAL_FAILURE_("Outcome of: " #expr)                       \
          << "  Actual:   Error '" << res.error().message() << "'\n"      \
          << "Expected:   Error '"                                        \
          << outcome::result<void>(error_id).error().message() << "'";    \
  } else                                                                  \
    GTEST_NONFATAL_FAILURE_("Outcome of: " #expr)                         \
        << "  Actual:   Success\n"                                        \
        << "Expected:   Error '"                                          \
        << outcome::result<void>(error_id).error().message() << "'";      \
  auto &&error = std::move(result.error());

#define ASSERT_OUTCOME_SUCCESS(variable, expression) \
  ASSERT_OUTCOME_SUCCESS_(TESTUTIL_OUTCOME_UNIQUE_NAME, variable, expression)

#define ASSERT_OUTCOME_SUCCESS_TRY(expression) \
  { ASSERT_OUTCOME_SUCCESS_TRY_(TESTUTIL_OUTCOME_UNIQUE_NAME, expression); }

#define ASSERT_OUTCOME_FAILURE(expression) \
  { ASSERT_OUTCOME_FAILURE_(TESTUTIL_OUTCOME_UNIQUE_NAME, expression); }

#define ASSERT_OUTCOME_ERROR(expression, error_id) \
  { ASSERT_OUTCOME_ERROR_(TESTUTIL_OUTCOME_UNIQUE_NAME, expression, error_id); }

#define EXPECT_OUTCOME_FAILURE(error, expression) \
  EXPECT_OUTCOME_FAILURE_(TESTUTIL_OUTCOME_UNIQUE_NAME, error, expression)

#define EXPECT_OUTCOME_ERROR(error, expression, error_id) \
  EXPECT_OUTCOME_ERROR_(                                  \
      TESTUTIL_OUTCOME_UNIQUE_NAME, error, expression, error_id)

#endif  // KAGOME_GTEST_OUTCOME_UTIL_HPP
