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
  EXPECT_FALSE(var);

#define EXPECT_OUTCOME_FALSE_name(var, val, expr) \
  auto &&var = expr;                              \
  EXPECT_FALSE(var);                              \
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

#define EXPECT_OUTCOME_RAISE_3(var, ecode, statement)                 \
  try {                                                               \
    statement;                                                        \
    FAIL() << "Line " << __LINE__ << ": " << #ecode << " not raised"; \
  } catch (std::system_error & var) {                                 \
    EXPECT_EQ(var.code(), ecode);                                     \
  }

#define EXPECT_OUTCOME_RAISE(ecode, statement) \
  EXPECT_OUTCOME_RAISE_3(UNIQUE_NAME(_e), ecode, statement)

#define EXPECT_OUTCOME_ERROR_3(var, ecode, expr) \
  {                                              \
    EXPECT_OUTCOME_FALSE_2(var, expr);           \
    EXPECT_EQ(var, ecode);                       \
  }

#define EXPECT_OUTCOME_EQ_3(var, expr, value) \
  {                                           \
    EXPECT_OUTCOME_TRUE_2(var, expr);         \
    EXPECT_EQ(var, value);                    \
  }

#define EXPECT_OUTCOME_EQ(expr, value) \
  EXPECT_OUTCOME_EQ_3(UNIQUE_NAME(_v), expr, value)

#define _OUTCOME_UNIQUE_NAME_GLUE2(x, y) x##y
#define _OUTCOME_UNIQUE_NAME_GLUE(x, y) _OUTCOME_UNIQUE_NAME_GLUE2(x, y)
#define _OUTCOME_UNIQUE_NAME \
  _OUTCOME_UNIQUE_NAME_GLUE(__outcome_result, __COUNTER__)

#define _ASSERT_OUTCOME_SUCCESS_TRY(_result_, _expression_)             \
  auto &&_result_ = (_expression_);                                     \
  if (not(_result_).has_value()) {                                      \
    GTEST_FATAL_FAILURE_("Outcome of: " #_expression_)                  \
        << "  Actual:   Error '" << _result_.error().message() << "'\n" \
        << "Expected:   Success";                                       \
  }

#define _ASSERT_OUTCOME_SUCCESS(_result_, _variable_, _expression_) \
  _ASSERT_OUTCOME_SUCCESS_TRY(_result_, _expression_);              \
  auto &&_variable_ = std::move(_result_.value());

#define ASSERT_OUTCOME_SUCCESS(_variable_, _expression_) \
  _ASSERT_OUTCOME_SUCCESS(_OUTCOME_UNIQUE_NAME, _variable_, _expression_)

#define ASSERT_OUTCOME_SUCCESS_TRY(_expression_) \
  { _ASSERT_OUTCOME_SUCCESS_TRY(_OUTCOME_UNIQUE_NAME, _expression_); }

#define ASSERT_OUTCOME_SOME_ERROR(_expression_)          \
  {                                                      \
    auto &&result = (_expression_);                      \
    if (not result.has_error()) {                        \
      GTEST_FATAL_FAILURE_("Outcome of: " #_expression_) \
          << "  Actual:   Success\n"                     \
          << "Expected:   Some error";                   \
    }                                                    \
  }

#define ASSERT_OUTCOME_ERROR(_expression_, _error_)                        \
  {                                                                        \
    auto &&result = (_expression_);                                        \
    if (result.has_error()) {                                              \
      if (result != outcome::failure(_error_)) {                           \
        GTEST_FATAL_FAILURE_("Outcome of: " #_expression_)                 \
            << "  Actual:   Error '" << result.error().message() << "'\n"  \
            << "Expected:   Error '" << make_error_code(_error_).message() \
            << "'";                                                        \
      }                                                                    \
    } else {                                                               \
      GTEST_FATAL_FAILURE_("Outcome of: " #_expression_)                   \
          << "  Actual:   Success\n"                                       \
          << "Expected:   Error '" << make_error_code(_error_).message()   \
          << "'";                                                          \
    }                                                                      \
  }

#define EXPECT_OUTCOME_SUCCESS(_result_, _expression_)                  \
  [[maybe_unused]] auto &&_result_ = (_expression_);                    \
  if (_result_.has_error()) {                                           \
    GTEST_NONFATAL_FAILURE_("Outcome of: " #_expression_)               \
        << "  Actual:   Error '" << _result_.error().message() << "'\n" \
        << "Expected:   Success";                                       \
  }

#define EXPECT_OUTCOME_SOME_ERROR(_result_, _expression_) \
  [[maybe_unused]] auto &&_result_ = (_expression_);      \
  if (not _result_.has_error()) {                         \
    GTEST_NONFATAL_FAILURE_("Outcome of: " #_expression_) \
        << "  Actual:   Success\n"                        \
        << "Expected:   Some error";                      \
  }

#define EXPECT_OUTCOME_ERROR(_result_, _expression_, _error_)                  \
  [[maybe_unused]] auto &&_result_ = (_expression_);                           \
  if (_result_.has_error()) {                                                  \
    if (_result_ != outcome::failure(_error_)) {                               \
      GTEST_NONFATAL_FAILURE_("Outcome of: " #_expression_)                    \
          << "  Actual:   Error '" << _result_.error().message() << "'\n"      \
          << "Expected:   Error '" << make_error_code(_error_).message()       \
          << "'";                                                              \
    }                                                                          \
  } else {                                                                     \
    GTEST_NONFATAL_FAILURE_("Outcome of: " #_expression_)                      \
        << "  Actual:   Success\n"                                             \
        << "Expected:   Error '" << make_error_code(_error_).message() << "'"; \
  }

#endif  // KAGOME_GTEST_OUTCOME_UTIL_HPP
