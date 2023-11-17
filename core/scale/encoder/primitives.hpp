/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_ENCODER_PRIMITIVES_HPP
#define KAGOME_SCALE_ENCODER_PRIMITIVES_HPP

#include <deque>
#include <map>
#include <optional>
#include <scale/outcome/outcome_throw.hpp>
#include <scale/types.hpp>
#include <span>
#include <tuple>
#include <type_traits>
#include <vector>

#include "utils/struct_to_tuple.hpp"

namespace kagome::scale {

  template <typename F>
  constexpr void putByte(const F &func, const uint8_t *const val, size_t count);

  template <typename F, typename... Ts>
  constexpr void encode(const F &func, const std::tuple<Ts...> &v);

  template <typename F, typename T, typename... Args>
  constexpr void encode(const F &func, const T &t, const Args &...args);

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::vector<T> &c);

  template <typename FN, typename F, typename S>
  constexpr void encode(const FN &func, const std::pair<F, S> &p);

  template <typename F, typename T, ssize_t S>
  constexpr void encode(const F &func, const std::span<T, S> &c);

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::span<T> &c);

  template <typename F, typename T, size_t size>
  constexpr void encode(const F &func, const std::array<T, size> &c);

  template <typename F, typename T, size_t N>
  constexpr void encode(const F &func, const T (&c)[N]);

  template <typename F, typename K, typename V>
  constexpr void encode(const F &func, const std::map<K, V> &c);

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::shared_ptr<T> &v);

  template <typename F>
  constexpr void encode(const F &func, const std::string_view &v);

  template <typename F>
  constexpr void encode(const F &func, const std::string &v);

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::unique_ptr<T> &v);

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::list<T> &c);

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::deque<T> &c);

  template <typename F, uint8_t I, typename... Ts>
  void encode(const F &func, const boost::variant<Ts...> &v);

  template <typename F, typename... Ts>
  void encode(const F &func, const boost::variant<Ts...> &v);

  template <typename F>
  void encode(const F &func, const ::scale::CompactInteger &value);

  template <typename F>
  void encode(const F &func, const ::scale::BitVec &value);

  template <typename F>
  void encode(const F &func, const std::optional<bool> &value);

  template <typename F, typename T>
  void encode(const F &func, const std::optional<T> &value);

  template <typename F,
            typename T,
            std::enable_if_t<!std::is_enum_v<std::decay_t<T>>, bool> = true>
  constexpr void encode(const F &func, const T &v) {
    using I = std::decay_t<T>;
    if constexpr (std::is_integral_v<I>) {
      if constexpr (std::is_same_v<I, bool>) {
        const uint8_t byte = (v ? 1u : 0u);
        putByte(func, &byte, 1ul);
        return;
      }

      if constexpr (sizeof(I) == 1u) {
        putByte(func, (const uint8_t *)&v, size_t(1ull));
        return;
      }

      constexpr size_t size = sizeof(I);
      const auto val = math::toLE(v);
      putByte(func, (uint8_t *)&val, size);
    } else {
      encode(func, utils::to_tuple_refs(v));
    }
  }

  template <typename F,
            typename T,
            std::enable_if_t<std::is_enum_v<std::decay_t<T>>, bool> = true>
  constexpr void encode(const F &func, const T &value) {
    encode(func, static_cast<std::underlying_type_t<std::decay_t<T>>>(value));
  }

  template <typename F, typename T, typename... Args>
  constexpr void encode(const F &func, const T &t, const Args &...args) {
    encode(func, t);
    encode(func, args...);
  }

  template <typename... Args>
  outcome::result<std::vector<uint8_t>> encode(const Args &...args) {
    std::vector<uint8_t> res;
    encode(
        [&](const uint8_t *const val, size_t count) {
          if (count != 0ull) {
            res.insert(res.end(), &val[0], &val[count]);
          }
        },
        args...);
    return res;
  }

  inline size_t bitUpperBorder(const ::scale::CompactInteger &x) {
    namespace mp = boost::multiprecision;
    const size_t size = x.backend().size();
    const mp::limb_type *const p = x.backend().limbs();

    auto counter = (size - 1) * sizeof(mp::limb_type) * 8;
    auto value = p[size - 1];

    if constexpr (sizeof(mp::limb_type) == sizeof(uint64_t)) {
      counter += sizeof(uint64_t) * 8
               - (value == 0 ? sizeof(uint64_t) * 8 : __builtin_clzll(value));
    } else if constexpr (sizeof(mp::limb_type) == sizeof(uint32_t)) {
      counter += sizeof(uint32_t) * 8
               - (value == 0 ? sizeof(uint32_t) * 8 : __builtin_clz(value));
    }
    return counter;
  }

  inline size_t countBytes(::scale::CompactInteger x) {
    if (x == 0) {
      return 1ull;
    }
    namespace mp = boost::multiprecision;
    const size_t size = x.backend().size();
    const mp::limb_type *const p = x.backend().limbs();

    auto counter = (size - 1) * sizeof(mp::limb_type);
    auto value = p[size - 1];

    static_assert(sizeof(mp::limb_type) >= sizeof(uint32_t),
                  "Unexpected limb size");
    if constexpr (sizeof(mp::limb_type) == sizeof(uint64_t)) {
      counter += 1 + ((sizeof(uint64_t) * 8 - __builtin_clzll(value)) - 1) / 8;
    } else if constexpr (sizeof(mp::limb_type) == sizeof(uint32_t)) {
      counter += 1 + ((sizeof(uint32_t) * 8 - __builtin_clz(value)) - 1) / 8;
    }
    return counter;
  }

  template <typename F>
  constexpr void putByte(const F &func,
                         const uint8_t *const val,
                         size_t count) {
    func(val, count);
  }

  template <typename F, uint8_t I, typename... Ts>
  void encode(const F &func, const boost::variant<Ts...> &v) {
    using T = std::tuple_element_t<I, std::tuple<Ts...>>;
    if (v.which() == I) {
      encode(func, I);
      encode(func, boost::get<T>(v));
      return;
    }
    if constexpr (sizeof...(Ts) > I + 1) {
      encode<F, I + 1>(func, v);
    }
  }

  template <typename F, typename T, size_t N>
  constexpr void encode(const F &func, const T (&c)[N]) {
    using E = std::decay_t<T>;
    if constexpr (std::is_integral_v<E> && sizeof(E) == 1u) {
      putByte(func, c, N);
    } else {
      for (const auto &e : c) {
        encode(func, e);
      }
    }
  }

  template <typename F, typename... Ts>
  void encode(const F &func, const boost::variant<Ts...> &v) {
    encode<F, 0>(func, v);
  }

  template <typename F,
            typename T,
            typename I = std::decay_t<T>,
            std::enable_if_t<std::is_unsigned_v<I>, bool> = true>
  constexpr void encodeCompactSmall(const F &func, T val) {
    BOOST_ASSERT_MSG((val >> (8 * sizeof(I) - 2)) == 0,
                     "Unexpected compact value in encoder");
    val <<= 2;
    val |= (sizeof(I) / 2ull);

    encode(func, val);
  }

  template <typename F>
  void encodeCompact(const F &func, uint64_t val) {
    if (val < ::scale::compact::EncodingCategoryLimits::kMinUint16) {
      encodeCompactSmall(func, static_cast<uint8_t>(val));
      return;
    }

    if (val < ::scale::compact::EncodingCategoryLimits::kMinUint32) {
      encodeCompactSmall(func, static_cast<uint16_t>(val));
      return;
    }

    if (val < ::scale::compact::EncodingCategoryLimits::kMinBigInteger) {
      encodeCompactSmall(func, static_cast<uint32_t>(val));
      return;
    }

    const size_t bigIntLength = sizeof(uint64_t) - (__builtin_clzll(val) / 8);

    uint8_t result[sizeof(uint64_t) + sizeof(uint8_t)];
    result[0] = (bigIntLength - 4) * 4 + 3;  // header

    *(uint64_t *)&result[1] = math::toLE(val);
    putByte(func, result, bigIntLength + 1ull);
  }

  template <typename F>
  constexpr void encode(const F &func, const std::string &v) {
    encode(func, std::string_view{v});
  }

  template <typename F>
  constexpr void encode(const F &func, const std::string_view &v) {
    encodeCompact(func, v.size());
    putByte(func, (const uint8_t *)v.data(), v.size());
  }

  template <typename F>
  void encode(const F &func, const ::scale::BitVec &v) {
    const size_t bitsCount = v.bits.size();
    const size_t bytesCount = ((bitsCount + 7ull) >> 3ull);
    const size_t blocksCount = ((bytesCount + 7ull) >> 3ull);

    encodeCompact(func, bitsCount);
    uint64_t result;
    size_t bitCounter = 0ull;
    for (size_t ix = 0ull; ix < blocksCount; ++ix) {
      result = 0ull;
      size_t remains = std::min(size_t(64ull), bitsCount - bitCounter);
      do {
        result |= ((v.bits[bitCounter] ? 1ull : 0ull) << (bitCounter % 64ull));
        ++bitCounter;
      } while (--remains);

      const size_t bits = (bitCounter % 64ull);
      const size_t bytes =
          (bits != 0ull) ? ((bits + 7ull) >> 3ull) : sizeof(result);

      result = math::toLE(result);
      putByte(func, (uint8_t *)&result, bytes);
    }
  }

  template <typename F>
  void encode(const F &func, const ::scale::CompactInteger &value) {
    if (value < 0) {
      raise(::scale::EncodeError::NEGATIVE_COMPACT_INTEGER);
    }

    const size_t bit_border = bitUpperBorder(value);
    if (bit_border <= 6) {  // kMinUint16
      encodeCompactSmall(func, value.convert_to<uint8_t>());
      return;
    }

    if (bit_border <= 14) {  // kMinUint32
      encodeCompactSmall(func, value.convert_to<uint16_t>());
      return;
    }

    if (bit_border <= 30) {  // kMinBigInteger
      encodeCompactSmall(func, value.convert_to<uint32_t>());
      return;
    }

    constexpr size_t kReserved = 68ull;
    const size_t bigIntLength = countBytes(value);

    if (bigIntLength >= kReserved) {
      raise(::scale::EncodeError::COMPACT_INTEGER_TOO_BIG);
    }

    namespace mp = boost::multiprecision;
    constexpr size_t limb_sz = sizeof(mp::limb_type);

    uint8_t result[1 + ((kReserved + limb_sz - 1) / limb_sz) * limb_sz];
    result[0] = (bigIntLength - 4) * 4 + 3;  // header

    const size_t size = value.backend().size();
    const mp::limb_type *const p = value.backend().limbs();

    size_t ix = 0ull;
    for (; ix < size; ++ix) {
      *(mp::limb_type *)&result[1ull + ix * limb_sz] = math::toLE(p[ix]);
    }

    putByte(func, result, bigIntLength + 1ull);
  }

  template <
      typename F,
      typename It,
      typename = std::enable_if_t<
          !std::is_same_v<typename std::iterator_traits<It>::value_type, void>>>
  constexpr void encode(const F &func, It begin, It end) {
    while (begin != end) {
      encode(func, *begin);
      ++begin;
    }
  }

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::span<T> &c) {
    encodeCompact(func, c.size());
    encode(func, c.begin(), c.end());
  }

  template <typename F, typename T, ssize_t S>
  constexpr void encode(const F &func, const std::span<T, S> &c) {
    if constexpr (S == -1) {
      encodeCompact(func, c.size());
      encode(func, c.begin(), c.end());
    } else {
      using E = std::decay_t<T>;
      if constexpr (std::is_integral_v<E> && sizeof(E) == 1u) {
        putByte(func, c.data(), c.size());
      } else {
        for (const auto &e : c) {
          encode(func, e);
        }
      }
    }
  }

  template <typename F, typename T, size_t size>
  constexpr void encode(const F &func, const std::array<T, size> &c) {
    for (const auto &e : c) {
      encode(func, e);
    }
  }

  template <typename F, typename K, typename V>
  constexpr void encode(const F &func, const std::map<K, V> &c) {
    encodeCompact(func, c.size());
    encode(func, c.begin(), c.end());
  }

  template <typename FN, typename F, typename S>
  constexpr void encode(const FN &func, const std::pair<F, S> &p) {
    encode(func, p.first);
    encode(func, p.second);
  }

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::vector<T> &c) {
    encodeCompact(func, c.size());
    encode(func, c.begin(), c.end());
  }

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::shared_ptr<T> &v) {
    if (v == nullptr) {
      raise(::scale::EncodeError::DEREF_NULLPOINTER);
    }
    encode(func, *v);
  }

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::unique_ptr<T> &v) {
    if (v == nullptr) {
      raise(::scale::EncodeError::DEREF_NULLPOINTER);
    }
    encode(func, *v);
  }

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::list<T> &c) {
    encodeCompact(func, c.size());
    encode(func, c.begin(), c.end());
  }

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::deque<T> &c) {
    encodeCompact(func, c.size());
    encode(func, c.begin(), c.end());
  }

  template <typename F, typename... Ts>
  constexpr void encode(const F &func, const std::tuple<Ts...> &v) {
    if constexpr (sizeof...(Ts) > 0) {
      std::apply([&](const auto &...s) { (..., encode(func, s)); }, v);
    }
  }

  template <typename F>
  void encode(const F &func, const std::optional<bool> &v) {
    enum class OptionalBool : uint8_t {
      NONE = 0u,
      OPT_TRUE = 1u,
      OPT_FALSE = 2u
    };

    auto result = OptionalBool::OPT_TRUE;
    if (!v.has_value()) {
      result = OptionalBool::NONE;
    } else if (!*v) {
      result = OptionalBool::OPT_FALSE;
    }
    encode(func, result);
  }

  template <typename F, typename T>
  void encode(const F &func, const std::optional<T> &v) {
    if (!v.has_value()) {
      encode(func, uint8_t(0u));
    } else {
      encode(func, uint8_t(1u));
      encode(func, *v);
    }
  }

}  // namespace kagome::scale

#endif  // KAGOME_SCALE_ENCODER_PRIMITIVES_HPP
