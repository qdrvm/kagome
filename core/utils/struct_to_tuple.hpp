/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STRUCT_TO_TUPLE_HPP
#define KAGOME_STRUCT_TO_TUPLE_HPP

#include <deque>
#include <map>
#include <optional>
#include <scale/outcome/outcome_throw.hpp>
#include <scale/types.hpp>
#include <tuple>
#include <type_traits>
#include <vector>

#define REP0(X)
#define REP1(X) X
#define REP2(X) REP1(X), X
#define REP3(X) REP2(X), X
#define REP4(X) REP3(X), X
#define REP5(X) REP4(X), X
#define REP6(X) REP5(X), X
#define REP7(X) REP6(X), X
#define REP8(X) REP7(X), X
#define REP9(X) REP8(X), X
#define REP10(X) REP9(X), X

#define REP0Y(X)
#define REP1Y(X) X##1
#define REP2Y(X) REP1Y(X), X##2
#define REP3Y(X) REP2Y(X), X##3
#define REP4Y(X) REP3Y(X), X##4
#define REP5Y(X) REP4Y(X), X##5
#define REP6Y(X) REP5Y(X), X##6
#define REP7Y(X) REP6Y(X), X##7
#define REP8Y(X) REP7Y(X), X##8
#define REP9Y(X) REP8Y(X), X##9

#define REP0Y_REF(X)
#define REP1Y_REF(X) std::cref(X##1)
#define REP2Y_REF(X) REP1Y_REF(X), std::cref(X##2)
#define REP3Y_REF(X) REP2Y_REF(X), std::cref(X##3)
#define REP4Y_REF(X) REP3Y_REF(X), std::cref(X##4)
#define REP5Y_REF(X) REP4Y_REF(X), std::cref(X##5)
#define REP6Y_REF(X) REP5Y_REF(X), std::cref(X##6)
#define REP7Y_REF(X) REP6Y_REF(X), std::cref(X##7)
#define REP8Y_REF(X) REP7Y_REF(X), std::cref(X##8)
#define REP9Y_REF(X) REP8Y_REF(X), std::cref(X##9)

#define REPEAT(TENS, ONES, X) REP##TENS(REP10(X)) REP##ONES(X)
#define REPEATY(ONES, X) REP##ONES##Y(X)
#define REPEATY_REF(ONES, X) REP##ONES##Y_REF(X)

#define TO_TUPLE_N(ONES)                                                      \
  if constexpr (is_braces_constructible<type, REPEAT(0, ONES, any_type)>{}) { \
    const auto &[REPEATY(ONES, p)] = object;                                  \
    return std::make_tuple(REPEATY_REF(ONES, p));                             \
  }

#define TO_TUPLE1             \
  TO_TUPLE_N(1) else {        \
    return std::make_tuple(); \
  }
#define TO_TUPLE2 TO_TUPLE_N(2) else TO_TUPLE1
#define TO_TUPLE3 TO_TUPLE_N(3) else TO_TUPLE2
#define TO_TUPLE4 TO_TUPLE_N(4) else TO_TUPLE3
#define TO_TUPLE5 TO_TUPLE_N(5) else TO_TUPLE4
#define TO_TUPLE6 TO_TUPLE_N(6) else TO_TUPLE5
#define TO_TUPLE7 TO_TUPLE_N(7) else TO_TUPLE6
#define TO_TUPLE8 TO_TUPLE_N(8) else TO_TUPLE7
#define TO_TUPLE9 TO_TUPLE_N(9) else TO_TUPLE8

namespace kagome::utils {

  template <typename T, typename... TArgs>
  decltype(void(T{std::declval<TArgs>()...}), std::true_type{})
  test_is_braces_constructible(int);
  template <typename, typename...>
  std::false_type test_is_braces_constructible(...);
  template <typename T, typename... TArgs>
  using is_braces_constructible =
      decltype(test_is_braces_constructible<T, TArgs...>(0));

  struct any_type {
    template <typename T>
    constexpr operator T();  // non explicit
  };

  template <typename T>
  auto to_tuple(const T &object) noexcept {
    using type = std::decay_t<T>;
    TO_TUPLE9;
  }
}  // namespace kagome::utils

namespace kagome::scale {

  template <typename F, typename T>
  constexpr void encode(const F &func, const T &v);

  template <typename F, typename... Ts>
  constexpr void encode(const F &func, const std::tuple<Ts...> &v);

  template <typename F, typename T, typename... Args>
  constexpr void encode(const F &func, const T &t, const Args &...args);

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::vector<T> &c);

  template <typename F, typename S>
  constexpr void encode(const F &func, const std::pair<F, S> &p);

  template <typename F, typename T, ssize_t S>
  constexpr void encode(const F &func, const gsl::span<T, S> &c);

  template <typename F, typename T, size_t size>
  constexpr void encode(const F &func, const std::array<T, size> &c);

  template <typename F, size_t N>
  constexpr void encode(const F &func, const char (&c)[N]);

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

  template <typename F, size_t I, typename... Ts>
  void encode(const F &func, const boost::variant<Ts...> &v);

  template <typename F, typename... Ts>
  void encode(const F &func, const boost::variant<Ts...> &v);

  template <typename F>
  void encode(const F &func, const ::scale::CompactInteger &value);

  template <typename F>
  void encode(const F &func, const ::scale::BitVec &value);

  inline size_t countBytes(::scale::CompactInteger v) {
    size_t counter = 0;
    do {
      ++counter;
    } while ((v >>= 8) != 0);
    return counter;
  }

  template<typename F>
  constexpr void putByte(const F &func, const uint8_t *const val, size_t count) {
    func(val, count);
  }

  template <typename F, uint8_t I, typename... Ts>
  void encode(const F &func, const boost::variant<Ts...> &v) {
    using T = std::tuple_element_t<I, std::tuple<Ts...>>;
    if (v.type() == typeid(T)) {
      encode(func, I);
      encode(func, boost::get<T>(v));
      return;
    }
    if constexpr (sizeof...(Ts) > I + 1) {
      encode<F, I + 1>(func, v);
    }
  }

  template <typename F, size_t N>
  constexpr void encode(const F &func, const char (&c)[N]) {
      for (const auto &e : c) {
        encode(func, e);
      }
  }

  template <typename F, typename... Ts>
  void encode(const F &func, const boost::variant<Ts...> &v) {
    encode<0>(func, v);
  }

  template <typename F, typename T,
            typename I = std::decay_t<T>,
            typename = std::enable_if_t<std::is_unsigned_v<I>>>
  constexpr void encodeCompact(const F &func, T val) {
    constexpr size_t sz = sizeof(I);
    constexpr uint8_t mask = 3 << 6;
    const uint8_t msb_byte = (uint8_t)(val >> ((sz - 1ull) * 8ull));

    BOOST_ASSERT_MSG((msb_byte & mask) == 0,
                     "Unexpected compact value in encoder");
    val <<= 2;
    val += (sz / 2ull);

    encode(func, val);
  }

    template <typename F>
  constexpr void encode(const F &func, const std::string &v) {
    encode(func, std::string_view{v});
  }

    template <typename F>
  constexpr void encode(const F &func, const std::string_view &v) {
    encode(func, ::scale::CompactInteger{v.size()});
    encode(func, v.begin(), v.end());
  }

  template <typename F>
  void encode(const F &func, const ::scale::BitVec &v) {
    const size_t bitsCount = v.bits.size();
    const size_t bytesCount = ((bitsCount + 7ull) >> 3ull);
    const size_t blocksCount = ((bytesCount + 7ull) >> 3ull);

    encode(func, ::scale::CompactInteger{bitsCount});
    uint64_t result;
    size_t bitCounter = 0ull;
    for (size_t ix = 0ull; ix < blocksCount; ++ix) {
      result = 0ull;
      size_t remains =
          (bitsCount - bitCounter) > 64ull ? 64ull : (bitsCount - bitCounter);
      do {
        result |= ((v.bits[bitCounter] ? 1ull : 0ull) << (bitCounter % 64ull));
        ++bitCounter;
      } while (--remains);

      const size_t bits = (bitCounter % 64ull);
      const size_t bytes =
          (bits != 0ull) ? ((bits + 7ull) >> 3ull) : sizeof(result);

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      if constexpr (size == 8) {
        v = __builtin_bswap64(v);
      } else if constexpr (size == 4) {
        v = __builtin_bswap32(v);
      } else if constexpr (size == 2) {
        v = __builtin_bswap16(v);
      } else {
        UNREACHABLE;
      }
#endif
      putByte(func, (uint8_t *)&result, bytes);
    }
  }

  template <typename F>
  void encode(const F &func, const ::scale::CompactInteger &value) {
    if (value < 0) {
      raise(::scale::EncodeError::NEGATIVE_COMPACT_INTEGER);
    }

    if (value < ::scale::compact::EncodingCategoryLimits::kMinUint16) {
      encodeCompact(func, value.convert_to<uint8_t>());
      return;
    }

    if (value < ::scale::compact::EncodingCategoryLimits::kMinUint32) {
      encodeCompact(func, value.convert_to<uint16_t>());
      return;
    }

    if (value < ::scale::compact::EncodingCategoryLimits::kMinBigInteger) {
      encodeCompact(func, value.convert_to<uint32_t>());
      return;
    }

    constexpr size_t kReserved = 68ull;
    const size_t bigIntLength = countBytes(value);

    if (bigIntLength >= kReserved) {
      raise(::scale::EncodeError::COMPACT_INTEGER_TOO_BIG);
    }

    uint8_t result[kReserved];
    result[0] = (bigIntLength - 4) * 4 + 3;  // header

    ::scale::CompactInteger v{value};
    size_t i = 0ull;
    for (; i < bigIntLength; ++i) {
      result[i + 1ull] = static_cast<uint8_t>(v & 0xFF);
      v >>= 8;
    }

    putByte(func, result, i);
  }

  template <typename F, 
      typename It,
      typename = std::enable_if_t<
          !std::is_same_v<typename std::iterator_traits<It>::value_type, void>>>
  constexpr void encode(const F &func, It begin, It end) {
    while (begin != end) {
      encode(func, *begin);
      ++begin;
    }
  }

  template <typename F, typename T, ssize_t S>
  constexpr void encode(const F &func, const gsl::span<T, S> &c) {
    if constexpr (S == -1) {
      encode(func, ::scale::CompactInteger{c.size()});
      encode(func, c.begin(), c.end());
    } else {
      for (const auto &e : c) {
        encode(func, e);
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
    encode(func, ::scale::CompactInteger{c.size()});
    encode(func, c.begin(), c.end());
  }

  template <typename F, typename S>
  constexpr void encode(const F &func, const std::pair<F, S> &p) {
    encode(func, p.first);
    encode(func, p.second);
  }

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::vector<T> &c) {
    encode(func, ::scale::CompactInteger{c.size()});
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
    encode(func, ::scale::CompactInteger{c.size()});
    encode(func, c.begin(), c.end());
  }

  template <typename F, typename T>
  constexpr void encode(const F &func, const std::deque<T> &c) {
    encode(func, ::scale::CompactInteger{c.size()});
    encode(func, c.begin(), c.end());
  }

  template <typename F, typename... Ts>
  constexpr void encode(const F &func, const std::tuple<Ts...> &v) {
    if constexpr (sizeof...(Ts) > 0) {
      std::apply([&](const auto &...s) { (..., encode(func, s)); }, v);
    }
  }

  template <typename F, typename T>
  constexpr void encode(const F &func, const T &v) {
    using I = std::decay_t<T>;
    if constexpr (std::is_integral_v<I>) {
      if constexpr (std::is_same_v<I, bool>) {
        const uint8_t byte = (v ? 1u : 0u);
        putByte(func, &byte, 1ul);
        return;
      }

      if constexpr (sizeof(I) == 1u) {
        putByte(func, &v, 1ul);
        return;
      }

      constexpr size_t size = sizeof(I);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      if constexpr (size == 8) {
        v = __builtin_bswap64(v);
      } else if constexpr (size == 4) {
        v = __builtin_bswap32(v);
      } else if constexpr (size == 2) {
        v = __builtin_bswap16(v);
      } else {
        UNREACHABLE;
      }
#endif

      putByte(func, (uint8_t *)&v, size);
    } else {
      encode(func, utils::to_tuple(v));
    }
  }

  template <typename F, typename T, typename... Args>
  constexpr void encode(const F &func, const T &t, const Args &...args) {
    encode(func, t);
    encode(func, args...);
  }

}  // namespace kagome::scale

#endif  // KAGOME_STRUCT_TO_TUPLE_HPP
