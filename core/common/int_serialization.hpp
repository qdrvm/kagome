/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_MP_UTILS_HPP
#define KAGOME_CORE_CRYPTO_MP_UTILS_HPP

#include <boost/multiprecision/cpp_int.hpp>
#include <gsl/span>

namespace kagome::common {

  using uint128_t = boost::multiprecision::uint128_t;
  using uint256_t = boost::multiprecision::uint256_t;

  std::array<uint8_t, 8> uint64_to_le_bytes(uint64_t number);
  uint64_t le_bytes_to_uint64(gsl::span<const uint8_t, 8> bytes);

  std::array<uint8_t, 8> uint64_to_be_bytes(uint64_t number);
  uint64_t be_bytes_to_uint64(gsl::span<const uint8_t, 8> bytes);

  std::array<uint8_t, 16> uint128_to_le_bytes(const uint128_t &i);
  uint128_t le_bytes_to_uint128(gsl::span<const uint8_t, 16> bytes);

  std::array<uint8_t, 16> uint128_to_be_bytes(const uint128_t &i);
  uint128_t be_bytes_to_uint128(gsl::span<const uint8_t, 16> bytes);

  std::array<uint8_t, 32> uint256_to_le_bytes(const uint256_t &i);
  uint256_t le_bytes_to_uint256(gsl::span<const uint8_t, 32> bytes);

  std::array<uint8_t, 32> uint256_to_be_bytes(const uint256_t &i);
  uint256_t be_bytes_to_uint256(gsl::span<const uint8_t, 32> bytes);

}  // namespace kagome::common

#endif  // KAGOME_CORE_CRYPTO_MP_UTILS_HPP
