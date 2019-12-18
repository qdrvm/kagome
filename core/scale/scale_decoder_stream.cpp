/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/scale_decoder_stream.hpp"

#include "common/outcome_throw.hpp"
#include "macro/unreachable.hpp"
#include "scale/scale_error.hpp"
#include "scale/types.hpp"

namespace kagome::scale {
  namespace {
    CompactInteger decodeCompactInteger(ScaleDecoderStream &stream) {
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
          }

          CompactInteger multiplier{1u};
          CompactInteger value = 0;
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

      return CompactInteger{number};
    }
  }  // namespace

  ScaleDecoderStream::ScaleDecoderStream(gsl::span<const uint8_t> span)
      : span_{span}, current_iterator_{span_.begin()}, current_index_{0} {}

  boost::optional<bool> ScaleDecoderStream::decodeOptionalBool() {
    auto byte = nextByte();
    switch (byte) {
      case static_cast<uint8_t>(OptionalBool::NONE):
        return boost::none;
        break;
      case static_cast<uint8_t>(OptionalBool::FALSE):
        return false;
      case static_cast<uint8_t>(OptionalBool::TRUE):
        return true;
      default:
        common::raise(DecodeError::UNEXPECTED_VALUE);
    }
    UNREACHABLE
  }

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

  ScaleDecoderStream &ScaleDecoderStream::operator>>(CompactInteger &v) {
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

  bool ScaleDecoderStream::hasMore(uint64_t n) const {
    return static_cast<SizeType>(current_index_ + n) <= span_.size();
  }

  uint8_t ScaleDecoderStream::nextByte() {
    if (not hasMore(1)) {
      common::raise(DecodeError::NOT_ENOUGH_DATA);
    }
    ++current_index_;
    return *current_iterator_++;
  }
}  // namespace kagome::scale
