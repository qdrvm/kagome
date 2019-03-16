/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_UTIL_HPP
#define KAGOME_SCALE_UTIL_HPP

#include <algorithm>
#include <cstdint>
#include <vector>

#include <boost/endian/arithmetic.hpp>

#include "common/scale/types.hpp"

namespace kagome::common::scale::impl {
  /**
   * encodeInteger encodes any integer type to little-endian representation
   * @tparam T integer type
   * @param value integer value
   * @return byte array representation of value
   */
  template <class T>
  ByteArray encodeInteger(T value) {
    constexpr size_t size = sizeof(T);
    static_assert(size >= 1, "types of size 0 are not supported");
    constexpr size_t bits = size * 8;
    boost::endian::endian_buffer<boost::endian::order::little, T, bits> buf{};
    buf = value;
    ByteArray result;
    result.reserve(size);
    for (auto i = 0; i < size; ++i) {
      result.push_back(buf.data()[i]);
    }
    return ByteArray(result);
  }

  /**
   * @brief decodeInteger function decodes integer from stream
   * @tparam T integer type
   * @param stream source stream
   * @return decoded value or error
   */
  template <class T>
  std::optional<T> decodeInteger(Stream &stream) {
    constexpr size_t size = sizeof(T);
    constexpr size_t bits = size * 8;
    static_assert(size == 1 || size == 2 || size == 4 || size == 8);

    // clang-format off
    // sign bit = 2^(num_bits - 1)
    static constexpr uint64_t sign_bit[] = {
            0x80,               // 1 byte
            0x8000,             // 2 bytes
            0x800000,           // 3 bytes
            0x80000000,         // 4 bytes
            0x8000000000,       // 5 bytes
            0x800000000000,     // 6 bytes
            0x80000000000000,   // 7 bytes
            0x8000000000000000  // 8 bytes
    };

    static constexpr uint64_t multiplier[] = {
            0x1,                // 2^0
            0x100,              // 2^8
            0x10000,            // 2^16
            0x1000000,          // 2^24
            0x100000000,        // 2^32
            0x10000000000,      // 2^40
            0x1000000000000,    // 2^48
            0x100000000000000   // 2^56
    };
    // clang-format on

    if (!stream.hasMore(size)) {
      return std::nullopt;
    }

    // get integer as 4 bytes from little-endian stream
    // and represent it as native-endian unsigned integer
    uint64_t v{0};
    for (auto i = 0; i < size; ++i) {
      v += multiplier[i] * static_cast<uint64_t>(*stream.nextByte());
    }
    // now we have uint64 native-endian value
    // which can be signed or unsigned under the cover

    // if it is unsigned, we know that is not greater than max value for type T
    // so static_cast<T>(v) is safe

    // if it is signed, but positive it is also ok
    // we can be sure that it is less than max_value<T>/2
    // to check whether is is negative we check if the sign bit present
    // in unsigned form it means that value is more than
    // a value 2^(bits_number-1)
    bool is_positive_signed = v < sign_bit[size - 1];
    if (std::is_unsigned<T>() || is_positive_signed) {
      return std::optional<T>(static_cast<T>(v));
    }

    // T is signed integer type and the value v is negative

    // value is negative signed means ( - x )
    // where x is positive unsigned < sign_bits[size-1]
    // find this x, safely cast to signed and negate result
    // the bitwise negation operation affects higher bits as well
    // but it doesn't spoil the result
    // static_cast to smaller size cuts them off
    T sv = -static_cast<T>((~v) + 1);

    return std::optional<T>{sv};
  }
}  // namespace kagome::common::scale::impl

#endif  // KAGOME_SCALE_UTIL_HPP
