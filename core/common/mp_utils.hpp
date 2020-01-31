/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_MP_UTILS_HPP
#define KAGOME_CORE_CRYPTO_MP_UTILS_HPP

#include <boost/multiprecision/cpp_int.hpp>
#include <gsl/span>

namespace kagome::common {

  namespace detail {
    template <size_t size, typename uint>
    std::array<uint8_t, size> uint_to_bytes(uint &&i);

    template <size_t size, typename uint>
    uint bytes_to_uint(gsl::span<uint8_t, size> bytes);
  }  // namespace detail

  std::array<uint8_t, 8> uint64_t_to_bytes(uint64_t number);

  uint64_t bytes_to_uint64_t(gsl::span<uint8_t, 8> bytes);

  std::array<uint8_t, 16> uint128_t_to_bytes(
      const boost::multiprecision::uint128_t &i);

  boost::multiprecision::uint128_t bytes_to_uint128_t(
      gsl::span<uint8_t, 16> bytes);

  std::array<uint8_t, 32> uint256_t_to_bytes(
      const boost::multiprecision::uint256_t &i);

  boost::multiprecision::uint256_t bytes_to_uint256_t(
      gsl::span<uint8_t, 32> bytes);

  template <size_t size, typename uint>
  inline std::array<uint8_t, size> encode256BE(const uint &value) {
    auto bytes = detail::uint_to_bytes<size>(value);
    decltype(bytes) buff;

    for (int i = 0; i < (int)bytes.size(); ++i) {
      int start = (int)buff.size() - (int)bytes.size() + i;
      if (start >= 0) {
        buff[start] = bytes[i];
      }
    }
    return buff;
  }

}  // namespace kagome::crypto::util

#endif  // KAGOME_CORE_CRYPTO_MP_UTILS_HPP
