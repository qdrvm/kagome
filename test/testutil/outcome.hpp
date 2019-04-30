/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GTEST_OUTCOME_UTIL_HPP
#define KAGOME_GTEST_OUTCOME_UTIL_HPP

#include <gtest/gtest.h>
#include <outcome/outcome.hpp>

#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a##b)
#define PP_CAT_II(p, res) res

#define UNIQUE_NAME(base) PP_CAT(base, __LINE__)

#define EXPECT_OUTCOME_TRUE_void(var, expr) \
  auto &&var = expr;                        \
  EXPECT_TRUE(var) << var.error().message();

#define EXPECT_OUTCOME_TRUE_name(var, val, expr) \
  auto &&var = expr;                             \
  EXPECT_TRUE(var) << var.error().message();     \
  auto &&val = var.value();

#define EXPECT_OUTCOME_FALSE_void(var, expr) \
  auto &&var = expr;                        \
  EXPECT_FALSE(var) << var.error().message();

#define EXPECT_OUTCOME_FALSE_name(var, val, expr) \
  auto &&var = expr;                             \
  EXPECT_FALSE(var) << var.error().message();     \
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

#define EXPECT_ERRCODE_SUCCESS(ec) EXPECT_FALSE(ec) << ec.message();

#endif  // KAGOME_GTEST_OUTCOME_UTIL_HPP
