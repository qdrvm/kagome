/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/util.hpp"

namespace kagome::crypto::util {

  namespace detail {
    template <size_t size, typename uint>
    std::array<uint8_t, size> uint_to_bytes(uint &&i) {
      auto count = i.backend().size();
      auto tsize = sizeof(boost::multiprecision::limb_type);
      auto copy_count = count * tsize;

      std::array<uint8_t, size> output{};
      memcpy(output.data(), i.backend().limbs(), copy_count);
      return output;
    }

    //    template <>
    //    std::array<uint8_t, 16> uint_to_bytes(
    //        const boost::multiprecision::uint128_t &);

    template <size_t size, typename uint>
    uint bytes_to_uint(gsl::span<uint8_t, size> bytes) {
      uint i;
      i.backend().resize(size, size);
      memcpy(i.backend().limbs(), bytes.data(), size);
      i.backend().normalize();
      return i;
    }
  }  // namespace detail

  std::array<uint8_t, 16> uint128_t_to_bytes(
      const boost::multiprecision::uint128_t &i) {
    return detail::uint_to_bytes<16>(i);
  }

  boost::multiprecision::uint128_t bytes_to_uint128_t(
      gsl::span<uint8_t, 16> bytes) {
    return detail::bytes_to_uint<16, boost::multiprecision::uint128_t>(bytes);
  }

  std::array<uint8_t, 32> uint256_t_to_bytes(
      const boost::multiprecision::uint256_t &i) {
    return detail::uint_to_bytes<32>(i);
  }

  boost::multiprecision::uint256_t bytes_to_uint256_t(
      gsl::span<uint8_t, 32> bytes) {
    return detail::bytes_to_uint<32, boost::multiprecision::uint256_t>(bytes);
  }

}  // namespace kagome::crypto::util
