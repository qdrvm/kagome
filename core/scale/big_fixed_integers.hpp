/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BIG_FIXED_INTEGERS_HPP
#define KAGOME_BIG_FIXED_INTEGERS_HPP

#include <type_traits>

#include <boost/multiprecision/cpp_int.hpp>
#include <scale/types.hpp>

#include "common/int_serialization.hpp"

namespace scale {

  template <typename T>
  struct IntegerTraits;

  /**
   * Wrapper for an integer to make distinct integer aliases
   * @tparam Tag
   * @tparam T
   */
  template <typename Tag, typename T>
  struct IntWrapper {
    static_assert(!std::is_reference_v<T>);
    static_assert(!std::is_pointer_v<T>);
    static_assert(!std::is_const_v<T>);

    using Self = IntWrapper<Tag, T>;
    using ValueType = T;

    static constexpr size_t BYTE_SIZE = IntegerTraits<T>::BIT_SIZE / 8;

    T &operator*() {
      return number;
    }

    const T &operator*() const {
      return number;
    }

    bool operator==(Self const &other) const {
      return **this == *other;
    }

    T number;
  };

  // TODO(Harrm) #1480 add check for narrowing conversion
  template <typename U,
            typename T,
            typename = std::enable_if_t<std::is_trivial_v<T>>>
  U convert_to(T t) {
    return static_cast<U>(t);
  }

  template <
      typename U,
      typename T,
      typename = std::enable_if_t<boost::multiprecision::is_number<T>::value>>
  U convert_to(T const &t) {
    try {
      return t.template convert_to<U>();
    } catch (std::runtime_error const &e) {
      // scale::decode catches std::system_errors
      throw std::system_error{
          make_error_code(std::errc::value_too_large),
          "This integer conversion would lead to information loss"};
    }
  }

  struct FixedTag {};

  // an integer intended to be encoded with fixed length
  template <typename T>
  using Fixed = IntWrapper<FixedTag, T>;

  struct CompactTag {};

  // an integer intended to be encoded with compact encoding
  template <typename T>
  using Compact = IntWrapper<CompactTag, T>;

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &stream, Fixed<N> &fixed) {
    for (size_t i = 0; i < Fixed<N>::BYTE_SIZE * 8; i += 8) {
      *fixed |= N(stream.nextByte()) << i;
    }
    return stream;
  }

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &stream, Fixed<N> const &fixed) {
    constexpr size_t bits = Fixed<N>::BYTE_SIZE * 8;
    for (size_t i = 0; i < bits; i += 8) {
      stream << convert_to<uint8_t>((*fixed >> i) & 0xFFu);
    }
    return stream;
  }

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &stream, Compact<N> &compact) {
    scale::CompactInteger n;
    stream >> n;
    compact.number = n.convert_to<N>();
    return stream;
  }

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &stream, Compact<N> const &compact) {
    scale::CompactInteger n = *compact;
    stream << n;
    return stream;
  }

#define SPECIALIZE_INTEGER_TRAITS(type, bit_size) \
  template <>                                     \
  struct IntegerTraits<type> {                    \
    static constexpr size_t BIT_SIZE = bit_size;  \
  };

  using uint128_t = boost::multiprecision::checked_uint128_t;
  using uint256_t = boost::multiprecision::checked_uint256_t;

  SPECIALIZE_INTEGER_TRAITS(uint8_t, 8);
  SPECIALIZE_INTEGER_TRAITS(uint16_t, 16);
  SPECIALIZE_INTEGER_TRAITS(uint32_t, 32);
  SPECIALIZE_INTEGER_TRAITS(uint64_t, 64);

  SPECIALIZE_INTEGER_TRAITS(uint128_t, 128);
  SPECIALIZE_INTEGER_TRAITS(uint256_t, 256);

#undef SPECIALIZE_INTEGER_TRAITS

}  // namespace scale

#endif  // KAGOME_BIG_FIXED_INTEGERS_HPP
