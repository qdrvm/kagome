/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OUTCOME_UTIL_HPP
#define KAGOME_OUTCOME_UTIL_HPP

#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a##b)
#define PP_CAT_II(p, res) res

#define UNIQUE_NAME(base) PP_CAT(base, __LINE__)

#define __OUTCOME_TRY_EC(var, expr) \
  auto &&var = expr;                \
  if (var) {                        \
    return var;                     \
  }

#define OUTCOME_TRY_EC(expr) __OUTCOME_TRY_EC(UNIQUE_NAME(_r), expr)

#endif  // KAGOME_OUTCOME_UTIL_HPP
