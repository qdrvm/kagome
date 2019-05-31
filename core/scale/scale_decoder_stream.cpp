/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/scale_decoder_stream.hpp"

#include "macro/unreachable.hpp"
#include "scale/outcome_throw.hpp"
#include "scale/scale_error.hpp"

namespace kagome::scale {
  namespace {
    BigInteger decodeCompactInteger(ScaleDecoderStream &stream) {
      auto first_byte = stream.nextByte();

      const uint8_t flag = (first_byte)&0b00000011u;

      size_t number = 0u;

      switch (flag) {
        case 0b00u: {
          number = static_cast<size_t>(first_byte >> 2u);
          break;
        }

        case 0b01u: {
          auto second_byte = stream.nextByte();

          number = (static_cast<size_t>((first_byte)&0b11111100u)
                    + static_cast<size_t>(second_byte) * 256u)
              >> 2u;
          break;
        }

        case 0b10u: {
          number = first_byte;
          size_t multiplier = 256u;
          if (!stream.hasMore(3u)) {
            // not enough data to decode integer
            common::raise(DecodeError::NOT_ENOUGH_DATA);
            UNREACHABLE
          }

          for (auto i = 0u; i < 3u; ++i) {
            // we assured that there are 3 more bytes,
            // no need to make checks in a loop
            number += (stream.nextByte()) * multiplier;
            multiplier = multiplier << 8u;
          }
          number = number >> 2u;
          break;
        }

        case 0b11: {
          auto bytes_count = ((first_byte) >> 2u) + 4u;
          if (!stream.hasMore(bytes_count)) {
            // not enough data to decode integer
            common::raise(DecodeError::NOT_ENOUGH_DATA);
            UNREACHABLE
          }

          BigInteger multiplier{1u};
          BigInteger value = 0;
          // we assured that there are m more bytes,
          // no need to make checks in a loop
          for (auto i = 0u; i < bytes_count; ++i) {
            value += (stream.nextByte()) * multiplier;
            multiplier *= 256u;
          }

          return value;  // special case
        }

        default:
          UNREACHABLE
      }

      return BigInteger{number};
    }
  }  // namespace

  ScaleDecoderStream::ScaleDecoderStream(gsl::span<const uint8_t> span)
      : current_ptr_{span.begin()}, end_ptr_{span.end()} {}

  bool ScaleDecoderStream::decodeBool() {
    auto byte = nextByte();

    switch (byte) {
      case 0u:
        return false;
      case 1u:
        return true;
      default:
        common::raise(DecodeError::UNEXPECTED_VALUE);
    }
    UNREACHABLE
  }

  ScaleDecoderStream &ScaleDecoderStream::operator>>(BigInteger &v) {
    v = decodeCompactInteger(*this);
    return *this;
  }

  ScaleDecoderStream &ScaleDecoderStream::operator>>(std::string &v) {
    std::vector<uint8_t> collection;
    *this >> collection;
    v.clear();
    v.append(collection.begin(), collection.end());
    return *this;
  }

  uint8_t ScaleDecoderStream::nextByte() {
    if (not hasMore(1)) {
      common::raise(DecodeError::NOT_ENOUGH_DATA);
      UNREACHABLE
    }
    return *current_ptr_++;
  }

  void ScaleDecoderStream::advance(uint64_t dist) {
    if (not hasMore(dist)) {
      common::raise(DecodeError::OUT_OF_BOUNDARIES);
      UNREACHABLE
    }
    current_ptr_ += dist;
  }

}  // namespace kagome::scale
