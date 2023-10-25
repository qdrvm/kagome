/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/int_serialization.hpp"

#include "macro/endianness_utils.hpp"

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
    inline uint le_bytes_to_uint(std::span<const uint8_t> bytes) {
      BOOST_ASSERT(bytes.size() >= size);
      uint result;
      import_bits(result, bytes.begin(), bytes.end(), 8, false);
      return result;
    }

    template <size_t size, typename uint>
    inline uint be_bytes_to_uint(std::span<const uint8_t> bytes) {
      BOOST_ASSERT(bytes.size() >= size);
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

  uint64_t le_bytes_to_uint64(std::span<const uint8_t> bytes) {
    BOOST_ASSERT(bytes.size() >= 8);
    uint64_t number;
    memcpy(&number, bytes.data(), 8);
    return le64toh(number);
  }

  std::array<uint8_t, 8> uint64_to_be_bytes(uint64_t number) {
    std::array<uint8_t, 8> result{};
    *reinterpret_cast<uint64_t *>(result.data()) = htobe64(number);
    return result;
  }

  uint64_t be_bytes_to_uint64(std::span<const uint8_t> bytes) {
    BOOST_ASSERT(bytes.size() >= 8);
    uint64_t number;
    memcpy(&number, bytes.data(), 8);
    return be64toh(number);
  }

  std::array<uint8_t, 16> uint128_to_le_bytes(
      const boost::multiprecision::uint128_t &i) {
    return uint_to_le_bytes<16>(i);
  }

  boost::multiprecision::uint128_t le_bytes_to_uint128(
      std::span<const uint8_t> bytes) {
    BOOST_ASSERT(bytes.size() >= 16);
    return le_bytes_to_uint<16, boost::multiprecision::uint128_t>(bytes);
  }

  std::array<uint8_t, 16> uint128_to_be_bytes(
      const boost::multiprecision::uint128_t &i) {
    return uint_to_be_bytes<16>(i);
  }

  boost::multiprecision::uint128_t be_bytes_to_uint128(
      std::span<const uint8_t> bytes) {
    BOOST_ASSERT(bytes.size() >= 16);
    return be_bytes_to_uint<16, boost::multiprecision::uint128_t>(bytes);
  }

  std::array<uint8_t, 32> uint256_to_le_bytes(
      const boost::multiprecision::uint256_t &i) {
    return uint_to_le_bytes<32>(i);
  }

  boost::multiprecision::uint256_t le_bytes_to_uint256(
      std::span<const uint8_t> bytes) {
    BOOST_ASSERT(bytes.size() >= 32);
    return le_bytes_to_uint<32, boost::multiprecision::uint256_t>(bytes);
  }

  std::array<uint8_t, 32> uint256_to_be_bytes(
      const boost::multiprecision::uint256_t &i) {
    return uint_to_be_bytes<32>(i);
  }

  boost::multiprecision::uint256_t be_bytes_to_uint256(
      std::span<const uint8_t> bytes) {
    BOOST_ASSERT(bytes.size() >= 32);
    return be_bytes_to_uint<32, boost::multiprecision::uint256_t>(bytes);
  }

}  // namespace kagome::common
