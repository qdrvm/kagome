/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace kagome::common;
using namespace std::string_literals;

struct UnhexNumber32Test
    : public ::testing::TestWithParam<std::pair<std::string, size_t>> {};

namespace {
  std::pair<std::string, size_t> makePair(std::string s, size_t v) {
    return std::make_pair(std::move(s), v);
  }
}  // namespace

TEST_P(UnhexNumber32Test, Unhex32Success) {
  auto &&[hex, val] = GetParam();
  EXPECT_OUTCOME_TRUE(decimal, unhexNumber<uint32_t>(hex));
  EXPECT_EQ(decimal, val);
}

INSTANTIATE_TEST_SUITE_P(UnhexNumberTestCases,
                         UnhexNumber32Test,
                         ::testing::Values(makePair("0x64", 100),
                                           makePair("0x01", 1),
                                           makePair("0xbc614e", 12345678)));
TEST(UnhexNumberTest, Overflow) {
  std::string encoded = "0x01FF";
  EXPECT_OUTCOME_ERROR(res,
                       unhexNumber<uint8_t>(encoded),
                       kagome::common::UnhexError::VALUE_OUT_OF_RANGE);
}

TEST(UnhexNumberTest, WrongFormat) {
  std::string encoded = "64";
  EXPECT_OUTCOME_FALSE_1(unhexNumber<uint8_t>(encoded));
}
