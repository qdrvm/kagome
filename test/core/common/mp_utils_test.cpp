/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/int_serialization.hpp"

#include <gtest/gtest.h>

#include <fmt/format.h>
#include <boost/multiprecision/cpp_int.hpp>

#include "common/buffer.hpp"
#include "macro/endianness_utils.hpp"

using namespace kagome::common;

#define ASSERT_TO_FROM_BYTES_EQUAL(value, integer_size)     \
  {                                                         \
    auto v = value;                                         \
    auto le_bytes = uint##integer_size##_to_le_bytes(v);    \
    ASSERT_EQ(le_bytes_to_uint##integer_size(le_bytes), v); \
    auto be_bytes = uint##integer_size##_to_be_bytes(v);    \
    ASSERT_EQ(be_bytes_to_uint##integer_size(be_bytes), v); \
  }

/**
 * @given a uint64
 * @when converting it to and then from bytes
 * @then the result matches with the original one
 */
TEST(MpUtilsTest, Uint64) {
  ASSERT_TO_FROM_BYTES_EQUAL(std::numeric_limits<uint64_t>::max(), 64);
  ASSERT_TO_FROM_BYTES_EQUAL(std::numeric_limits<uint64_t>::min(), 64);
  ASSERT_TO_FROM_BYTES_EQUAL(static_cast<uint32_t>(1), 64);
  ASSERT_TO_FROM_BYTES_EQUAL(static_cast<uint32_t>(1) << 31, 64);
  ASSERT_TO_FROM_BYTES_EQUAL(1337, 64);
}

/**
 * @given a uint128
 * @when converting it to and then from bytes
 * @then the result matches with the original one
 */
TEST(MpUtilsTest, Uint128) {
  ASSERT_TO_FROM_BYTES_EQUAL(std::numeric_limits<uint128_t>::max(), 128);
  ASSERT_TO_FROM_BYTES_EQUAL(std::numeric_limits<uint128_t>::min(), 128);
  ASSERT_TO_FROM_BYTES_EQUAL(static_cast<uint64_t>(1), 128);
  ASSERT_TO_FROM_BYTES_EQUAL(static_cast<uint64_t>(1) << 63, 128);
  ASSERT_TO_FROM_BYTES_EQUAL(
      static_cast<uint128_t>(std::numeric_limits<uint64_t>::max()) * 4 + 1,
      128);
  ASSERT_TO_FROM_BYTES_EQUAL(1337, 128);
}

/**
 * @given a uint256
 * @when converting it to and then from bytes
 * @then the result matches with the original one
 */
TEST(MpUtilsTest, Uint256) {
  ASSERT_TO_FROM_BYTES_EQUAL(std::numeric_limits<uint256_t>::max(), 256);
  ASSERT_TO_FROM_BYTES_EQUAL(std::numeric_limits<uint256_t>::min(), 256);
  ASSERT_TO_FROM_BYTES_EQUAL(static_cast<uint64_t>(1), 256);
  ASSERT_TO_FROM_BYTES_EQUAL(static_cast<uint64_t>(1) << 63, 256);
  ASSERT_TO_FROM_BYTES_EQUAL(
      static_cast<uint256_t>(std::numeric_limits<uint128_t>::max()) * 4 + 1,
      256);
  ASSERT_TO_FROM_BYTES_EQUAL(1337, 256);
}

/**
 * @given a uint256
 * @when converting it to and then from bytes
 * @then the result matches with the original one
 */
TEST(MpUtilsTest, DISABLED_View) {
  {
    auto x = uint64_t(1);
    auto le = uint64_to_le_bytes(x);
    std::cout << fmt::format("le64 x => {:l}\n", kagome::common::Buffer(le));
    auto be = uint64_to_be_bytes(x);
    std::cout << fmt::format("be64 x => {:l}\n", kagome::common::Buffer(be));
  }
  {
    auto x = uint128_t(1);
    auto le = uint128_to_le_bytes(x);
    std::cout << fmt::format("le128 x => {:l}\n", kagome::common::Buffer(le));
    auto be = uint128_to_be_bytes(x);
    std::cout << fmt::format("be128 x => {:l}\n", kagome::common::Buffer(be));
  }
  {
    auto x = uint256_t(1);
    auto le = uint256_to_le_bytes(x);
    std::cout << fmt::format("le256 x => {:l}\n", kagome::common::Buffer(le));
    auto be = uint256_to_be_bytes(x);
    std::cout << fmt::format("be256 x => {:l}\n", kagome::common::Buffer(be));
  }

  {
    auto x = uint64_t(0xffu << 24);
    auto le = uint64_to_le_bytes(x);
    std::cout << fmt::format("le64 x => {:l}\n", kagome::common::Buffer(le));
    auto be = uint64_to_be_bytes(x);
    std::cout << fmt::format("be64 x => {:l}\n", kagome::common::Buffer(be));
  }
  {
    auto x = uint128_t(0xffu << 24);
    auto le = uint128_to_le_bytes(x);
    std::cout << fmt::format("le128 x => {:l}\n", kagome::common::Buffer(le));
    auto be = uint128_to_be_bytes(x);
    std::cout << fmt::format("be128 x => {:l}\n", kagome::common::Buffer(be));
  }
  {
    auto x = uint256_t(0xffu << 24);
    auto le = uint256_to_le_bytes(x);
    std::cout << fmt::format("le256 x => {:l}\n", kagome::common::Buffer(le));
    auto be = uint256_to_be_bytes(x);
    std::cout << fmt::format("be256 x => {:l}\n", kagome::common::Buffer(be));
  }
}

/**
 * @given bigint value and known serialized representation of it
 * @when serialize bigint to bytes
 * @then expected serialized bytes are returned
 */
TEST(MpUtilsTest, UInt128ConvertTest) {
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

  auto a_encoded = uint128_to_le_bytes(a);
  ASSERT_EQ(encoded, a_encoded) << "a = " << a;

  auto a_decoded = le_bytes_to_uint128(a_encoded);
  ASSERT_EQ(a_decoded, a);
}
