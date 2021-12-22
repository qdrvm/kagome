/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/mp_utils.hpp"

#include <gsl/gsl_util>

namespace kagome::common {

  namespace {
    template <size_t size, typename uint>
    inline std::array<uint8_t, size> uint_to_le_bytes(uint &&i) {
      std::array<uint8_t, size> res{};
      res.fill(0);
      export_bits(i, res.begin(), 8, false);
      return res;
    }

    template <size_t size, typename uint>
    inline std::array<uint8_t, size> uint_to_be_bytes(uint &&i) {
      std::array<uint8_t, size> res{};
      res.fill(0);
      export_bits(i, res.rbegin(), 8, false);
      return res;
    }

    template <size_t size, typename uint>
    inline uint le_bytes_to_uint(gsl::span<uint8_t, size> bytes) {
      if (bytes.empty()) {
        return uint(0);
      }
      uint result;
      import_bits(result, bytes.begin(), bytes.end(), 8, false);
      return result;
    }

    template <size_t size, typename uint>
    inline uint be_bytes_to_uint(gsl::span<uint8_t, size> bytes) {
      if (bytes.empty()) {
        return uint(0);
      }
      uint result;
      import_bits(result, bytes.rbegin(), bytes.rend(), 8, false);
      return result;
    }
  }  // namespace

  std::array<uint8_t, 8> uint64_to_le_bytes(uint64_t number) {
    std::array<uint8_t, 8> result{};
    *reinterpret_cast<uint64_t *>(result.data()) = htole64(number);
    return result;
  }

  uint64_t le_bytes_to_uint64(gsl::span<uint8_t, 8> bytes) {
    uint64_t number;
    memcpy(&number, bytes.data(), 8);
    return le64toh(number);
  }

  std::array<uint8_t, 8> uint64_to_be_bytes(uint64_t number) {
    std::array<uint8_t, 8> result{};
    *reinterpret_cast<uint64_t *>(result.data()) = htobe64(number);
    return result;
  }

  uint64_t be_bytes_to_uint64(gsl::span<uint8_t, 8> bytes) {
    uint64_t number;
    memcpy(&number, bytes.data(), 8);
    return be64toh(number);
  }

  std::array<uint8_t, 16> uint128_to_le_bytes(
      const boost::multiprecision::uint128_t &i) {
    return uint_to_le_bytes<16>(i);
  }

  boost::multiprecision::uint128_t le_bytes_to_uint128(
      gsl::span<uint8_t, 16> bytes) {
    return le_bytes_to_uint<16, boost::multiprecision::uint128_t>(bytes);
  }

  std::array<uint8_t, 16> uint128_to_be_bytes(
      const boost::multiprecision::uint128_t &i) {
    return uint_to_be_bytes<16>(i);
  }

  boost::multiprecision::uint128_t be_bytes_to_uint128(
      gsl::span<uint8_t, 16> bytes) {
    return be_bytes_to_uint<16, boost::multiprecision::uint128_t>(bytes);
  }

  std::array<uint8_t, 32> uint256_to_le_bytes(
      const boost::multiprecision::uint256_t &i) {
    return uint_to_le_bytes<32>(i);
  }

  boost::multiprecision::uint256_t le_bytes_to_uint256(
      gsl::span<uint8_t, 32> bytes) {
    return le_bytes_to_uint<32, boost::multiprecision::uint256_t>(bytes);
  }

  std::array<uint8_t, 32> uint256_to_be_bytes(
      const boost::multiprecision::uint256_t &i) {
    return uint_to_be_bytes<32>(i);
  }

  boost::multiprecision::uint256_t be_bytes_to_uint256(
      gsl::span<uint8_t, 32> bytes) {
    return be_bytes_to_uint<32, boost::multiprecision::uint256_t>(bytes);
  }

}  // namespace kagome::common
