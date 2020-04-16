/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

}  // namespace kagome::common

#endif  // KAGOME_CORE_CRYPTO_MP_UTILS_HPP
