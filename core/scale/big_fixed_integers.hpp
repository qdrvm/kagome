/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <type_traits>

#include <boost/multiprecision/cpp_int.hpp>
#include <scale/scale.hpp>
#include <scale/types.hpp>

#include "common/tagged.hpp"

namespace scale {

  using uint128_t = boost::multiprecision::checked_uint128_t;
  using uint256_t = boost::multiprecision::checked_uint256_t;

  template <typename T>
  concept SupportedInteger = std::is_unsigned_v<T>         //
                          or std::is_same_v<T, uint128_t>  //
                          or std::is_same_v<T, uint256_t>;

  template <SupportedInteger T>
  struct IntegerTraits {
    static constexpr size_t kBitSize = sizeof(T) * 8;
  };
  template <>
  struct IntegerTraits<uint128_t> {
    static constexpr size_t kBitSize = 128;
  };
  template <>
  struct IntegerTraits<uint256_t> {
    static constexpr size_t kBitSize = 256;
  };

  // TODO(Harrm) #1480 add check for narrowing conversion
  template <typename To, typename From>
    requires std::is_trivial_v<From>
  To convert_to(From t) {
    return static_cast<To>(t);
  }

  template <typename To, typename From>
    requires boost::multiprecision::is_number<From>::value
  To convert_to(const From &t) {
    try {
      return t.template convert_to<To>();
    } catch (const std::runtime_error &e) {
      // scale::decode catches std::system_errors
      throw std::system_error{
          make_error_code(std::errc::value_too_large),
          "This integer conversion would lead to information loss"};
    }
  }

  // an integer intended to be encoded with fixed length
  template <typename T>
    requires SupportedInteger<T>
  using Fixed = kagome::Tagged<T, struct FixedTag>;

  // an integer intended to be encoded with compact encoding
  template <typename T>
    requires SupportedInteger<T>
  using Compact = qtils::Tagged<T, struct CompactTag>;

  template <typename T>
  ScaleDecoderStream &operator>>(ScaleDecoderStream &stream, Fixed<T> &fixed) {
    T decoded = 0;
    for (size_t i = 0; i < IntegerTraits<T>::kBitSize; i += 8) {
      decoded |= T(stream.nextByte()) << i;
    }
    fixed = decoded;
    return stream;
  }

  template <typename T>
  ScaleEncoderStream &operator<<(ScaleEncoderStream &stream,
                                 const Fixed<T> &fixed) {
    T original = untagged(fixed);
    for (size_t i = 0; i < IntegerTraits<T>::kBitSize; i += 8) {
      stream << convert_to<uint8_t>((original >> i) & 0xFFu);
    }
    return stream;
  }

  template <typename N>
  ScaleDecoderStream &operator>>(ScaleDecoderStream &stream,
                                 Compact<N> &compact) {
    scale::CompactInteger n;
    stream >> n;
    compact = n.convert_to<N>();
    return stream;
  }

  template <typename N>
  ScaleEncoderStream &operator<<(ScaleEncoderStream &stream,
                                 const Compact<N> &compact) {
    scale::CompactInteger n = untagged(compact);
    stream << n;
    return stream;
  }

}  // namespace scale
