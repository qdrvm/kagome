/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/mp_utils.hpp"

#include <gtest/gtest.h>

#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::uint128_t;
using boost::multiprecision::uint256_t;
using kagome::common::bytes_to_uint128_t;
using kagome::common::bytes_to_uint256_t;
using kagome::common::uint128_t_to_bytes;
using kagome::common::uint256_t_to_bytes;

#define ASSERT_TO_FROM_BYTES_EQUAL(value, integer_size)    \
  {                                                        \
    auto v = value;                                        \
    auto v_bytes = uint##integer_size##_t_to_bytes(v);     \
    ASSERT_EQ(bytes_to_uint##integer_size##_t(v_bytes), v); \
  }

TEST(MpUtilsTest, Uint128) {
  ASSERT_TO_FROM_BYTES_EQUAL(std::numeric_limits<uint128_t>::max(), 128);
  ASSERT_TO_FROM_BYTES_EQUAL(std::numeric_limits<uint128_t>::min(), 128);
  ASSERT_TO_FROM_BYTES_EQUAL(static_cast<uint128_t>(std::numeric_limits<uint64_t>::max())*4+1, 128);
  ASSERT_TO_FROM_BYTES_EQUAL(1337, 128);
}

TEST(MpUtilsTest, Uint256) {
  auto max = std::numeric_limits<uint256_t>::max();
  auto max_bytes = uint256_t_to_bytes(max);
  ASSERT_EQ(bytes_to_uint256_t(max_bytes), max);

  auto min = std::numeric_limits<uint256_t>::min();
  auto min_bytes = uint256_t_to_bytes(min);
  ASSERT_EQ(bytes_to_uint256_t(min_bytes), min);
}

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
