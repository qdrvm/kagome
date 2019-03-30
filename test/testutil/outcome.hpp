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

#define __EXPECT_OUTCOME_TRUE_2(var, expr)                  \
  auto &&var = expr;                                        \
  EXPECT_TRUE(var) << var.error().category().name() << ": " \
                   << var.error().message();

#define __EXPECT_OUTCOME_TRUE_3(var, val, expr) \
  __EXPECT_OUTCOME_TRUE_2(var, expr)            \
  auto &&val = var.value();


/**
 * Use this macro in GTEST with 2 arguments to assert that getResult()
 * returned VALUE and immediately get this value.
 * EXPECT_OUTCOME_TRUE(val, getResult());
 */
#define EXPECT_OUTCOME_TRUE(val, expr) \
  __EXPECT_OUTCOME_TRUE_3(UNIQUE_NAME(_r), val, expr)


#endif  // KAGOME_GTEST_OUTCOME_UTIL_HPP
