/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

// BASED ON BITCOIN IMPLEMENTATION WITH THE FOLLOWING COPYRIGHT:
//
// Copyright (c) 2014-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/base58_codec.hpp"

/** All alphanumeric characters except for "0", "I", "O", and "l" */
static const char *pszBase58 =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static const int8_t mapBase58[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,
    8,  -1, -1, -1, -1, -1, -1, -1, 9,  10, 11, 12, 13, 14, 15, 16, -1, 17, 18,
    19, 20, 21, -1, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1,
    -1, -1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

OUTCOME_CPP_DEFINE_CATEGORY(kagome::primitives, Base58Error, e) {
  using E = kagome::primitives::Base58Error;
  switch (e) {
    case E::INVALID_CHARACTER:
      return "Invalid character in a Base58 string";
    case E::NULL_TERMINATOR_NOT_FOUND:
      return "Encountered a non null terminated Base58 string";
  }
  return "Unknown error in base58 decoder";
}

namespace kagome::primitives {

  outcome::result<common::Buffer> decodeBase58(std::string_view str) noexcept {
    // Skip leading spaces.
    while (str.length() > 0 && std::isspace(str[0])) {
      str = str.substr(1);
    }
    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;
    while (str[0] == '1') {
      zeroes++;
    }
    str = str.substr(zeroes);

    // Allocate enough space in big-endian base256 representation.
    int size =
        str.length() * 733 / 1000 + 1;  // log(58) / log(256), rounded up.
    std::vector<unsigned char> b256(size);

    // Process the characters.
    static_assert(
        sizeof(mapBase58) / sizeof(mapBase58[0]) == 256,
        "mapBase58.size() should be 256");  // guarantee not out of range

    while (str.length() > 0 && !std::isspace(str[0])) {
      // Decode base58 character
      int carry = mapBase58[static_cast<uint8_t>(str[0])];
      if (carry == -1)  // Invalid b58 character
        return Base58Error::INVALID_CHARACTER;
      int i = 0;
      for (auto it = b256.rbegin();
           (carry != 0 || i < length) && (it != b256.rend());
           ++it, ++i) {
        carry += 58 * (*it);
        *it = carry % 256;
        carry /= 256;
      }
      BOOST_ASSERT(carry == 0);
      length = i;
      str = str.substr(1);
    }
    // Skip trailing spaces.
    while (std::isspace(str[0])) {
      str = str.substr(1);
    }
    if (str[0] != 0) return Base58Error::NULL_TERMINATOR_NOT_FOUND;
    // Skip leading zeroes in b256.
    auto it = b256.begin() + (size - length);

    // Copy result into output vector.
    common::Buffer res{};
    res.reserve(zeroes + (b256.end() - it));
    std::fill_n(res.begin(), zeroes, 0);
    while (it != b256.end()) res.putUint8(*(it++));
    return res;
  }

  std::string encodeBase58(gsl::span<const uint8_t> input) noexcept {
    // Skip & count leading zeroes.
    int zeroes = 0;
    int length = 0;
    while (input.size() > 0 && input[0] == 0) {
      input = input.subspan(1);
      zeroes++;
    }
    // Allocate enough space in big-endian base58 representation.
    int size = input.size() * 138 / 100 + 1;  // log(256) / log(58), rounded up.
    std::vector<unsigned char> b58(size);
    // Process the bytes.
    while (input.size() > 0) {
      int carry = input[0];
      int i = 0;
      // Apply "b58 = b58 * 256 + ch".
      for (auto it = b58.rbegin();
           (carry != 0 || i < length) && (it != b58.rend());
           it++, i++) {
        carry += 256 * (*it);
        *it = carry % 58;
        carry /= 58;
      }

      BOOST_ASSERT(carry == 0);
      length = i;
      input = input.subspan(1);
    }
    // Skip leading zeroes in base58 result.
    auto it = b58.begin() + (size - length);
    it = std::find_if(it, b58.end(), [](auto b) { return b != 0; });

    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end()) {
      str += pszBase58[*(it++)];
    }
    return str;
  }

}  // namespace kagome::primitives
