/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "primitives/base58_codec.hpp"
#include "testutil/outcome.hpp"

constexpr size_t base58_pair_num = 14;
constexpr const char *base58_pairs[base58_pair_num][2]{
    {"", ""},
    {"61", "2g"},
    {"626262", "a3gV"},
    {"636363", "aPEr"},
    {"73696d706c792061206c6f6e6720737472696e67",
     "2cFupjhnEsSn59qHXstmK2ffpLv2"},
    {"00eb15231dfceb60925886b67d065299925915aeb172c06647",
     "1NS17iag9jJgTHD1VXjvLCEnZuQ3rJDE9L"},
    {"516b6fcd0f", "ABnLTmg"},
    {"bf4f89001e670274dd", "3SEo3LWLoPntC"},
    {"572e4794", "3EFU7m"},
    {"ecac89cad93923c02321", "EJDM8drfXA6uyA"},
    {"10c8511e", "Rt5zm"},
    {"00000000000000000000", "1111111111"},
    {"000111d38e5fc9071ffcd20b4a763cc9ae4f252bb4e48fd66a835e252ada93ff480d6dd43"
     "dc62a641155a5",
     "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"},
    {"000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232"
     "425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748"
     "494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6"
     "d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f9091"
     "92939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b"
     "6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9da"
     "dbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfef"
     "f",
     "1cWB5HCBdLjAuqGGReWE3R3CguuwSjw6RHn39s2yuDRTS5NsBgNiFpWgAnEx6VQi8csexkgYw"
     "3mdYrMHr8x9i7aEwP8kZ7vccXWqKDvGv3u1GxFKPuAkn8JCPPGDMf3vMMnbzm6Nh9zh1gcNsM"
     "vH3ZNLmP5fSG6DGbbi2tuwMWPthr4boWwCxf7ewSgNQeacyozhKDDQQ1qL5fQFUW52QKUZDZ5"
     "fw3KXNQJMcNTcaB723LchjeKun7MuGW5qyCBZYzA1KjofN1gYBV3NqyhQJ3Ns746GNuf9N2pQ"
     "PmHz4xpnSrrfCvy6TVVz5d4PdrjeshsWQwpZsZGzvbdAdN8MKV5QsBDY"}};

using E = kagome::primitives::Base58Error;
using namespace std::string_literals;

constexpr size_t base58_test_num = 12;
const struct Base58TestStr {
  std::string str;
  E err = E::INVALID_CHARACTER;
  enum class State { OK, FAIL } state = State::FAIL;
} base58_str[base58_test_num]{
    {"invalid"},
    {"invalid\0"s},
    {"\0invalid"s},
    {"good", {}, Base58TestStr::State::OK},
    {"bad0IOl"},
    {"goodbad0IOl"},
    {"good\0bad0IOl"s},
    // check that DecodeBase58 skips whitespace, but still fails with unexpected
    // non-whitespace at the end.
    {" \t\n\v\f\r skip \r\f\v\n\t a", E::NULL_TERMINATOR_NOT_FOUND},
    {" \t\n\v\f\r skip \r\f\v\n\t ", {}, Base58TestStr::State::OK},
    {"3vQB7B6MrGQZaxCuFg4oh", {}, Base58TestStr::State::OK},
    {"3vQB7B6MrGQZaxCuFg4oh0IOl"},
    {"3vQB7B6MrGQZaxCuFg4oh\0"
     "0IOl"s}};

TEST(Base58, Decode) {
  for (size_t i = 0; i < base58_pair_num; ++i) {
    EXPECT_OUTCOME_SUCCESS(
        res, kagome::primitives::decodeBase58(base58_pairs[i][1]));
    EXPECT_EQ(res, kagome::common::Buffer::fromHex(base58_pairs[i][0]));
  }
}

TEST(Base58, Encode) {
  for (size_t i = 0; i < base58_pair_num; ++i) {
    auto ref = kagome::common::Buffer::fromHex(base58_pairs[i][0]).value();
    EXPECT_EQ(kagome::primitives::encodeBase58(
                  gsl::make_span(ref.data(), ref.size())),
              std::string(base58_pairs[i][1]));
  }
}

TEST(Base58, Check) {
  for (size_t i = 0; i < base58_test_num; ++i) {
    std::cout << base58_str[i].str << std::endl;
    auto str = kagome::primitives::decodeBase58(std::string(base58_str[i].str));
    if (Base58TestStr::State::OK == base58_str[i].state) {
      EXPECT_OUTCOME_SUCCESS(res, str);
    } else {
      EXPECT_OUTCOME_ERROR(res, str, base58_str[i].err);
    }
  }
}
