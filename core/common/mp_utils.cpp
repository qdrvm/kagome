/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/mp_utils.hpp"

#include <boost/endian/conversion.hpp>
#include <gsl/gsl_util>

namespace kagome::common {

  namespace detail {
    template <size_t size, typename uint>
    std::array<uint8_t, size> uint_to_bytes(uint &&i) {
      using boost::multiprecision::cpp_int;
      using boost::endian::native_to_little_inplace;
      std::array<uint8_t, size> res {};
      res.fill(0);
      export_bits(i, res.begin(), 8, false);
      native_to_little_inplace(res);
      return res;
    }

    template <size_t size, typename uint>
    uint bytes_to_uint(gsl::span<uint8_t, size> bytes) {
      using boost::multiprecision::cpp_int;
      using boost::endian::little_to_native;
      if (bytes.empty()) {
        return uint(0);
      }
      auto le = little_to_native(bytes);
      uint result;
      import_bits(result, le.begin(), le.end(), 8, false);
      return result;
    }
  }  // namespace detail

  std::array<uint8_t, 8> uint64_t_to_bytes(uint64_t number) {
    std::array<uint8_t, 8> result{};
    for (auto i = 0; i < 8; ++i) {
      gsl::at(result, i) = static_cast<uint8_t>((number >> 8 * (7 - i)) & 0xFF);
    }
    return result;
  }

  uint64_t bytes_to_uint64_t(gsl::span<uint8_t, 8> bytes) {
    uint64_t result{0};
    for (auto i = 0; i < 8; ++i) {
      result |= static_cast<uint64_t>(bytes[i]) << (i * 8);
    }
    return result;
  }

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
