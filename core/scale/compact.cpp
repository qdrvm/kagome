/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/compact.hpp"

#include <algorithm>
#include <limits>
#include <tuple>
#include <type_traits>

#include "common/unreachable.hpp"
#include "scale/scale_error.hpp"
#include "scale/util.hpp"

namespace kagome::scale::compact {
  struct EncodingCategoryLimits {
    // min integer encoded by 2 bytes
    constexpr static size_t kMinUint16 = (1ul << 6);
    // min integer encoded by 4 bytes
    constexpr static size_t kMinUint32 = (1ul << 14);
    // min integer encoded as multibyte
    constexpr static size_t kMinBigInteger = (1ul << 30);
  };

  namespace impl {
    outcome::result<void> encodeFirstCategory(uint8_t value,
                                              common::Buffer &out) {
      if (value >= EncodingCategoryLimits::kMinUint16) {
        return EncodeError::kWrongCategory;
      }
      // only values from [0, kMinUint16) can be put here
      out.putUint8(static_cast<uint8_t>(value << 2));

      return outcome::success();
    };

    outcome::result<void> encodeSecondCategory(uint16_t value,
                                               common::Buffer &out) {
      if (value >= EncodingCategoryLimits::kMinUint32) {
        return EncodeError::kWrongCategory;
      };
      // only values from [kMinUint16, kMinUint32) can be put here
      auto v = value;
      v <<= 2;  // v *= 4
      v += 1;   // set 0b01 flag
      auto minor_byte = static_cast<uint8_t>(v & 0xFF);
      v >>= 8;
      auto major_byte = static_cast<uint8_t>(v & 0xFF);
      out.putUint8(minor_byte);
      out.putUint8(major_byte);

      return outcome::success();
      ;
    };

    outcome::result<void> encodeThirdCategory(uint32_t value,
                                              common::Buffer &out) {
      if (value >= EncodingCategoryLimits::kMinBigInteger) {
        return EncodeError::kWrongCategory;
      };

      uint32_t v = (value << 2) + 2;

      scale::impl::encodeInteger<uint32_t>(v, out);

      return outcome::success();
    }
  };  // namespace impl

  namespace {
    // calculate number of bytes required
    size_t countBytes(BigInteger v) {
      if (v == 0) {
        return 1;
      }

      size_t counter = 0;
      while (v > 0) {
        ++counter;
        v >>= 8;
      }

      return counter;
    };
  }  // namespace

  outcome::result<void> encodeInteger(const BigInteger &value,
                                      common::Buffer &out) {
    // cannot encode negative numbers
    // there is no description how to encode compact negative numbers
    if (value < 0) {
      return EncodeError::kCompactIntegerIsNegative;
    }

    if (value < EncodingCategoryLimits::kMinUint16) {
      return impl::encodeFirstCategory(value.convert_to<uint8_t>(), out);
    }

    if (value < EncodingCategoryLimits::kMinUint32) {
      return impl::encodeSecondCategory(value.convert_to<uint16_t>(), out);
    }

    if (value < EncodingCategoryLimits::kMinBigInteger) {
      return impl::encodeThirdCategory(value.convert_to<uint32_t>(), out);
    }

    // number of bytes required to represent value
    size_t bigIntLength = countBytes(value);

    // number of bytes to scale-encode value
    // 1 byte is reserved for header
    size_t requiredLength = 1 + bigIntLength;

    if (bigIntLength > 67) {
      return EncodeError::kCompactIntegerIsTooBig;
    }

    ByteArray result;
    result.reserve(requiredLength);

    /* The value stored in 6 major bits of header is used
     * to encode number of bytes for storing big integer.
     * Value formed by 6 bits varies from 0 to 63 == 2^6 - 1,
     * However big integer byte count starts from 4,
     * so to store this number we should decrease this value by 4.
     * And the range of bytes number for storing big integer
     * becomes 4 .. 67. To form resulting header we need to move
     * those bits representing bytes count to the left by 2 positions
     * by means of multiplying by 4.
     * Minor 2 bits store encoding option, in our case it is 0b11 == 3
     * We just add 3 to the result of operations above
     */
    uint8_t header = (bigIntLength - 4) * 4 + 3;

    result.push_back(header);

    BigInteger v{value};
    for (size_t i = 0; i < bigIntLength; ++i) {
      result.push_back(
          static_cast<uint8_t>(v & 0xFF));  // push back least significant byte
      v >>= 8;
    }

    out.put(result);

    return outcome::success();
  }

  outcome::result<BigInteger> decodeInteger(common::ByteStream &stream) {
    auto first_byte = stream.nextByte();
    if (!first_byte.has_value()) {
      return outcome::failure(DecodeError::kNotEnoughData);
    }

    const uint8_t flag = (*first_byte) & 0b00000011;

    size_t number = 0;

    switch (flag) {
      case 0b00: {
        number = static_cast<size_t>((*first_byte) >> 2);
        break;
      }

      case 0b01: {
        auto second_byte = stream.nextByte();
        if (!second_byte.has_value()) {
          return outcome::failure(DecodeError::kNotEnoughData);
        }

        number = (static_cast<size_t>((*first_byte) & 0b11111100)
                  + static_cast<size_t>(*second_byte) * 256)
            >> 2;
        break;
      }

      case 0b10: {
        number = *first_byte;
        size_t multiplier = 256;
        if (!stream.hasMore(3)) {
          // not enough data to decode integer
          return outcome::failure(DecodeError::kNotEnoughData);
        }

        for (int i = 0; i < 3; ++i) {
          // we assured that there are 3 more bytes,
          // no need to make checks in a loop
          number += (*stream.nextByte()) * multiplier;
          multiplier = multiplier << 8;
        }
        number = number >> 2;
        break;
      }

      case 0b11: {
        auto bytes_count = ((*first_byte) >> 2) + 4;
        if (!stream.hasMore(bytes_count)) {
          // not enough data to decode integer
          return outcome::failure(DecodeError::kNotEnoughData);
        }

        BigInteger multiplier{1};
        BigInteger value = 0;
        // we assured that there are m more bytes,
        // no need to make checks in a loop
        for (auto i = 0; i < bytes_count; ++i) {
          value += (*stream.nextByte()) * multiplier;
          multiplier *= 256;
        }

        return value;  // special case
      }

      default:
        UNREACHABLE
    }

    return BigInteger{number};
  }
}  // namespace kagome::scale::compact
