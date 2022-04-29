/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/threshold_util.hpp"

#include <gtest/gtest.h>

using namespace kagome::consensus;  // NOLINT
using namespace kagome;             // NOLINT

/**
 * Same test and outputs as in go:
 * https://github.com/ChainSafe/gossamer/blob/b3053c9222d113477abd86e50dc58faa78ec51ce/lib/babe/babe_test.go#L121
 *
 * @given arguments for calculateThreshold (c, authorities list weights,
 * authority index)
 * @when calculateThreshold with provided arguments
 * @then expected known threshold is returned
 */
TEST(ThresholdTest, OutputAsInGossamer) {
  std::pair<uint64_t, uint64_t> c;
  c.first = 5;
  c.second = 17;
  primitives::AuthorityIndex authority_index{3};
  primitives::AuthorityList authorities;
  authorities.push_back(primitives::Authority{.id = {}, .weight = 3});
  authorities.push_back(primitives::Authority{.id = {}, .weight = 1});
  authorities.push_back(primitives::Authority{.id = {}, .weight = 4});
  authorities.push_back(primitives::Authority{.id = {}, .weight = 6});
  authorities.push_back(primitives::Authority{.id = {}, .weight = 10});

  Threshold expected{"28377230912881121443596276039380107264"};
  auto threshold = calculateThreshold(c, authorities, authority_index);
  ASSERT_EQ(threshold, expected);
}
