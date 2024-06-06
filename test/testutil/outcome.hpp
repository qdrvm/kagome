/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>
#include "common/visitor.hpp"
#include "outcome/outcome.hpp"

#define EXPECT_OUTCOME_TRUE_void(var, expr)       \
  auto &&var = expr;                              \
  EXPECT_TRUE(var) << "Line " << __LINE__ << ": " \
                   << fmt::to_string(var.error());

#define EXPECT_OUTCOME_TRUE_name(var, val, expr) \
  EXPECT_OUTCOME_TRUE_void(var, expr);           \
  auto &&val = var.value();

#define EXPECT_OUTCOME_FALSE_void(var, expr) \
  auto &&var = expr;                         \
  EXPECT_FALSE(var);

#define EXPECT_OUTCOME_FALSE_name(var, val, expr) \
  auto &&var = expr;                              \
  EXPECT_FALSE(var);                              \
  auto &&val = var.error();

#define EXPECT_OUTCOME_TRUE_1(expr) \
  EXPECT_OUTCOME_TRUE_void(OUTCOME_UNIQUE, expr)

#define EXPECT_OUTCOME_FALSE_1(expr) \
  EXPECT_OUTCOME_FALSE_void(OUTCOME_UNIQUE, expr)

/**
 * Use this macro in GTEST with 2 arguments to assert that getResult()
 * returned VALUE and immediately get this value.
 * EXPECT_OUTCOME_TRUE(val, getResult());
 */
#define EXPECT_OUTCOME_TRUE(val, expr) \
  EXPECT_OUTCOME_TRUE_name(OUTCOME_UNIQUE, val, expr)

#define EXPECT_OUTCOME_FALSE(val, expr) \
  EXPECT_OUTCOME_FALSE_name(OUTCOME_UNIQUE, val, expr)

// TODO(turuslan): #2047, remove ambigous duplicates
#define ASSERT_OUTCOME_ERROR EXPECT_EC
#define ASSERT_OUTCOME_SUCCESS EXPECT_OUTCOME_TRUE
#define ASSERT_OUTCOME_SUCCESS_TRY EXPECT_OUTCOME_TRUE_1
#define EXPECT_OUTCOME_ERROR(unused, expr, expected) EXPECT_EC(expr, expected)
#define EXPECT_OUTCOME_SUCCESS(tmp, expr) EXPECT_OUTCOME_TRUE_void(tmp, expr)

#define _EXPECT_EC(tmp, expr, expected) \
  auto &&tmp = expr;                    \
  EXPECT_TRUE(tmp.has_error());         \
  EXPECT_EQ(tmp.error(), expected);
#define EXPECT_EC(expr, expected) _EXPECT_EC(OUTCOME_UNIQUE, expr, expected)
