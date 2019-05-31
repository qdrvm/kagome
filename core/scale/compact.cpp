///**
// * Copyright Soramitsu Co., Ltd. All Rights Reserved.
// * SPDX-License-Identifier: Apache-2.0
// */
//
//#include "scale/compact.hpp"
//
//#include <algorithm>
//#include <limits>
//#include <tuple>
//#include <type_traits>
//
//#include "outcome_throw.hpp"
//#include "macro/unreachable.hpp"
//#include "scale/scale_encoder_stream.hpp"
//#include "scale/scale_error.hpp"
//#include "scale/types.hpp"
//#include "scale/detail/fixed_witdh_integer.hpp"
//
//namespace kagome::scale::compact {
//  // TODO(yuraz): PRE-119 rework this function as operator
//  // and move it to scale_decoder_stream.cpp
//
//  outcome::result<BigInteger> decodeInteger(ScaleDecoderStream &stream) {
//    auto first_byte = stream.nextByte();
//    if (!first_byte.has_value()) {
//      return outcome::failure(DecodeError::NOT_ENOUGH_DATA);
//    }
//
//    const uint8_t flag = (*first_byte) & 0b00000011u;
//
//    size_t number = 0u;
//
//    switch (flag) {
//      case 0b00u: {
//        number = static_cast<size_t>((*first_byte) >> 2u);
//        break;
//      }
//
//      case 0b01u: {
//        auto second_byte = stream.nextByte();
//        if (!second_byte.has_value()) {
//          return outcome::failure(DecodeError::NOT_ENOUGH_DATA);
//        }
//
//        number = (static_cast<size_t>((*first_byte) & 0b11111100u)
//                  + static_cast<size_t>(*second_byte) * 256u)
//            >> 2u;
//        break;
//      }
//
//      case 0b10u: {
//        number = *first_byte;
//        size_t multiplier = 256u;
//        if (!stream.hasMore(3u)) {
//          // not enough data to decode integer
//          return outcome::failure(DecodeError::NOT_ENOUGH_DATA);
//        }
//
//        for (auto i = 0u; i < 3u; ++i) {
//          // we assured that there are 3 more bytes,
//          // no need to make checks in a loop
//          number += (*stream.nextByte()) * multiplier;
//          multiplier = multiplier << 8u;
//        }
//        number = number >> 2u;
//        break;
//      }
//
//      case 0b11: {
//        auto bytes_count = ((*first_byte) >> 2u) + 4u;
//        if (!stream.hasMore(bytes_count)) {
//          // not enough data to decode integer
//          return outcome::failure(DecodeError::NOT_ENOUGH_DATA);
//        }
//
//        BigInteger multiplier{1u};
//        BigInteger value = 0;
//        // we assured that there are m more bytes,
//        // no need to make checks in a loop
//        for (auto i = 0u; i < bytes_count; ++i) {
//          value += (*stream.nextByte()) * multiplier;
//          multiplier *= 256u;
//        }
//
//        return value;  // special case
//      }
//
//      default:
//        UNREACHABLE
//    }
//
//    return BigInteger{number};
//  }
//}  // namespace kagome::scale::compact
