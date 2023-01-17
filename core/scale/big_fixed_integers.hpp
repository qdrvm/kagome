/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BIG_FIXED_INTEGERS_HPP
#define KAGOME_BIG_FIXED_INTEGERS_HPP

#include <type_traits>

#include <boost/multiprecision/cpp_int.hpp>

#include "common/int_serialization.hpp"

namespace scale {

  template <typename Tag, typename T>
  struct IntWrapper {
    using Self = IntWrapper<Tag, T>;

    T &operator*() {
      return number;
    }

    const T &operator*() const {
      return number;
    }

    bool operator==(Self const& other) const {
      return **this == *other;
    }

    T number;
  };

  struct FixedTag {};

  template<typename T>
  using Fixed = IntWrapper<FixedTag, T>;

  struct CompactTag {};

  template<typename T>
  using Compact = IntWrapper<CompactTag, T>;

  template <typename T>
  struct EncodeAsFixed {
    explicit EncodeAsFixed(T &number) : number{number} {}
    T &number;
  };

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_decoder_stream>,
            typename = std::enable_if_t<std::numeric_limits<N>::is_integer>>
  Stream &operator>>(Stream &stream, Fixed<N>& fixed) {
    return stream >> EncodeAsFixed{*fixed};
  }

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_encoder_stream>,
            typename = std::enable_if_t<std::numeric_limits<N>::is_integer>>
  Stream &operator<<(Stream &stream, Compact<N> compact) {
    scale::CompactInteger n = compact;
    stream << n;
    return stream;
  }

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_decoder_stream>,
            typename = std::enable_if_t<std::numeric_limits<N>::is_integer>>
  Stream &operator>>(Stream &stream, Compact<N> &compact) {
    scale::CompactInteger n;
    stream >> n;
    compact.number = n.convert_to<N>();
    return stream;
  }

  template <typename T>
  struct EncodeAsCompact {
    explicit EncodeAsCompact(T &number) : number{number} {}
    T &number;
  };

  template <typename T>
  struct BigIntegerTraits {
    static constexpr bool value = false;

    static constexpr size_t BIT_SIZE = sizeof(T) * 8;
  };

  using uint128_t = boost::multiprecision::uint128_t;
  using uint256_t = boost::multiprecision::uint256_t;

  template <>
  struct BigIntegerTraits<uint128_t> {
    static constexpr size_t BIT_SIZE = 128;
    static constexpr bool value = true;
  };

  template <>
  struct BigIntegerTraits<uint256_t> {
    static constexpr size_t BIT_SIZE = 256;
    static constexpr bool value = true;
  };

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_decoder_stream>,
            typename = std::enable_if_t<BigIntegerTraits<N>::value>>
  Stream &operator>>(Stream &, N &) {
    static_assert(!BigIntegerTraits<N>::value,  // just for dependent context
                  "You need to wrap your number in EncodeAsFixed or "
                  "EncodeAsCompact to clarify the encoding");
  }

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_decoder_stream>,
            typename = std::enable_if_t<BigIntegerTraits<N>::value>>
  Stream &operator>>(Stream &stream, EncodeAsFixed<N> fixed) {
    for (size_t i = 0; i < BigIntegerTraits<N>::BIT_SIZE; i+=8) {
      fixed.number |= N(stream.nextByte()) << (i);
    }
    return stream;
  }

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_encoder_stream>,
            typename = std::enable_if_t<BigIntegerTraits<N>::value>>
  Stream &operator<<(Stream &stream, EncodeAsFixed<N> fixed) {
    constexpr size_t bits = BigIntegerTraits<N>::BIT_SIZE;
    for (N i = 0; i < bits; i += 8) {
      stream << fixed & 0xFF;
      fixed >>= 8;
    }
    return stream;
  }

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_decoder_stream>,
            typename = std::enable_if_t<BigIntegerTraits<N>::value>>
  Stream &operator>>(Stream &stream, EncodeAsCompact<N> compact) {
    scale::CompactInteger n;
    stream >> n;
    compact.number = n;
    return stream;
  }

  template <typename Stream,
            typename N,
            typename = std::enable_if_t<Stream::is_encoder_stream>,
            typename = std::enable_if_t<BigIntegerTraits<N>::value>>
  Stream &operator<<(Stream &stream, EncodeAsCompact<N> compact) {
    scale::CompactInteger n = compact;
    stream << n;
    return stream;
  }

}  // namespace scale

#endif  // KAGOME_BIG_FIXED_INTEGERS_HPP
