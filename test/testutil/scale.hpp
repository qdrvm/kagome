/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_SCALE_HPP
#define KAGOME_TEST_TESTUTIL_SCALE_HPP

#include "scale/scale.hpp"

#include "common/hexutil.hpp"
#include "testutil/outcome.hpp"

using kagome::common::hex_upper;

/**
 * Check that:
 * - scale encoding value yields expected bytes;
 * - scale encoding value decoded from bytes yields same bytes;
 */
template <typename T>
void expectEncodeAndReencode(const T &value,
                             const std::vector<uint8_t> &expected_bytes) {
  EXPECT_OUTCOME_TRUE(actual_bytes, kagome::scale::encode(value));
  EXPECT_EQ(actual_bytes, expected_bytes)
      << "actual bytes: " << hex_upper(actual_bytes) << std::endl
      << "expected bytes: " << hex_upper(expected_bytes);

  EXPECT_OUTCOME_TRUE(decoded, kagome::scale::decode<T>(expected_bytes));
  EXPECT_OUTCOME_EQ(kagome::scale::encode(decoded), expected_bytes);
}

#endif  // KAGOME_TEST_TESTUTIL_SCALE_HPP
