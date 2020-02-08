/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/mp_utils.hpp"
#include "common/blob.hpp"

#include <gtest/gtest.h>

using namespace kagome::common;

/**
 * @given bigint value and known serialized representation of it
 * @when serialize bigint to bytes
 * @then expected serialized bytes are returned
 */
TEST(MPUtilsTest, UInt128ConvertTest) {
  boost::multiprecision::uint128_t a{"4961875008018162238211470133173564236"};

  std::array<uint8_t, 16> encoded;
  encoded[0] = 'L';
  encoded[1] = '3';
  encoded[2] = '\xa2';
  encoded[3] = '\n';
  encoded[4] = 'C';
  encoded[5] = '\xf4';
  encoded[6] = '5';
  encoded[7] = '\x93';
  encoded[8] = '\xc5';
  encoded[9] = '\x05';
  encoded[10] = '\xe0';
  encoded[11] = ']';
  encoded[12] = 'S';
  encoded[13] = '\x9f';
  encoded[14] = '\xbb';
  encoded[15] = '\x03';

  auto a_encoded = uint128_t_to_bytes(a);
  ASSERT_EQ(encoded, a_encoded) << "a = " << a;

  auto a_decoded = bytes_to_uint128_t(a_encoded);
  ASSERT_EQ(a_decoded, a);
}
