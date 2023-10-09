/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/big_fixed_integers.hpp"

#include <gtest/gtest.h>
#include <scale/scale.hpp>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using scale::Compact;
using scale::Fixed;
using scale::uint128_t;

template <template <typename T> typename Wrapper, typename Number>
auto encode_compare_decode(const Number &given_number,
                           const Buffer &desired_encoding) {
  auto fixed_number = Wrapper<Number>{given_number};
  EXPECT_OUTCOME_TRUE(encoded, scale::encode(fixed_number));
  ASSERT_EQ(encoded, desired_encoding);
  EXPECT_OUTCOME_TRUE(decoded, scale::decode<Wrapper<Number>>(encoded));
  ASSERT_EQ(fixed_number, decoded);
}

// Google Test doesn't support parameterizing tests by value and by type
// simultaneously
#define DEFINE_TEST_SUITE(type, encoding, ...)                       \
                                                                     \
  class type##_##encoding##_##IntegerTest                            \
      : public testing::TestWithParam<std::pair<type, Buffer>> {};   \
                                                                     \
  TEST_P(type##_##encoding##_##IntegerTest, EncodeCompareDecode) {   \
    auto &[given_number, desired_encoding] = GetParam();             \
    encode_compare_decode<encoding>(given_number, desired_encoding); \
  }                                                                  \
                                                                     \
  INSTANTIATE_TEST_SUITE_P(                                          \
      type, type##_##encoding##_##IntegerTest, testing::Values(__VA_ARGS__));

DEFINE_TEST_SUITE(uint128_t,
                  Fixed,
                  std::pair{0, "00000000000000000000000000000000"_hex2buf},
                  std::pair{1, "01000000000000000000000000000000"_hex2buf},
                  std::pair{42, "2A000000000000000000000000000000"_hex2buf},
                  std::pair{std::numeric_limits<uint128_t>::max(),
                            "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"_hex2buf});
DEFINE_TEST_SUITE(uint32_t,
                  Fixed,
                  std::pair{0, "00000000"_hex2buf},
                  std::pair{1, "01000000"_hex2buf},
                  std::pair{42, "2A000000"_hex2buf},
                  std::pair{std::numeric_limits<uint32_t>::max(),
                            "FFFFFFFF"_hex2buf});
DEFINE_TEST_SUITE(uint128_t,
                  Compact,
                  std::pair{0, "00"_hex2buf},
                  std::pair{1, "04"_hex2buf},
                  std::pair{42, "A8"_hex2buf},
                  std::pair{std::numeric_limits<uint128_t>::max(),
                            "33FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"_hex2buf});
DEFINE_TEST_SUITE(uint32_t,
                  Compact,
                  std::pair{0, "00"_hex2buf},
                  std::pair{1, "04"_hex2buf},
                  std::pair{42, "A8"_hex2buf},
                  std::pair{std::numeric_limits<uint32_t>::max(),
                            "03FFFFFFFF"_hex2buf});
