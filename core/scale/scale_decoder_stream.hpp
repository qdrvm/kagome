/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_SCALE_DECODER_STREAM_HPP
#define KAGOME_CORE_SCALE_SCALE_DECODER_STREAM_HPP

#include <array>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <gsl/span>

#include "common/outcome_throw.hpp"
#include "scale/detail/fixed_witdh_integer.hpp"

namespace kagome::scale {
  class ScaleDecoderStream {
   public:
    // special tag to differentiate decoding streams from others
    static constexpr auto is_decoder_stream = true;

    explicit ScaleDecoderStream(gsl::span<const uint8_t> span);

    /**
     * @brief scale-decodes pair of values
     * @tparam F first value type
     * @tparam S second value type
     * @param p pair of values to decode
     * @return reference to stream
     */
    template <class F, class S>
    ScaleDecoderStream &operator>>(std::pair<F, S> &p) {
      static_assert(!std::is_reference_v<F> && !std::is_reference_v<S>);
      return *this >> const_cast<std::remove_const_t<F> &>(p.first)  // NOLINT
             >> const_cast<std::remove_const_t<S> &>(p.second);      // NOLINT
    }

    /**
     * @brief scale-decoding of tuple
     * @tparam T enumeration of tuples types
     * @param v reference to tuple
     * @return reference to stream
     */
    template <class... T>
    ScaleDecoderStream &operator>>(std::tuple<T...> &v) {
      if constexpr (sizeof...(T) > 0) {
        decodeElementOfTuple<0>(v);
      }
      return *this;
    }

    /**
     * @brief scale-decoding of variant
     * @tparam T enumeration of various types
     * @param v reference to variant
     * @return reference to stream
     */
    template <class... Ts>
    ScaleDecoderStream &operator>>(boost::variant<Ts...> &v) {
      // first byte means type index
      uint8_t type_index = 0u;
      *this >> type_index;  // decode type index

      // ensure that index is in [0, types_count)
      if (type_index >= sizeof...(Ts)) {
        common::raise(DecodeError::WRONG_TYPE_INDEX);
      }

      tryDecodeAsOneOfVariant<0>(v, type_index);
      return *this;
    }

    /**
     * @brief scale-decodes shared_ptr value
     * @tparam T value type
     * @param v value to decode
     * @return reference to stream
     */
    template <class T>
    ScaleDecoderStream &operator>>(std::shared_ptr<T> &v) {
      using mutableT = std::remove_const_t<T>;

      static_assert(std::is_default_constructible_v<mutableT>);

      v = std::make_shared<mutableT>();
      return *this >> const_cast<mutableT &>(*v);  // NOLINT
    }

    /**
     * @brief scale-decodes unique_ptr value
     * @tparam T value type
     * @param v value to decode
     * @return reference to stream
     */
    template <class T>
    ScaleDecoderStream &operator>>(std::unique_ptr<T> &v) {
      using mutableT = std::remove_const_t<T>;

      static_assert(std::is_default_constructible_v<mutableT>);

      v = std::make_unique<mutableT>();
      return *this >> const_cast<mutableT &>(*v);  // NOLINT
    }

    /**
     * @brief scale-encodes any integral type including bool
     * @tparam T integral type
     * @param v value of integral type
     * @return reference to stream
     */
    template <typename T,
              typename I = std::decay_t<T>,
              typename = std::enable_if_t<std::is_integral<I>::value>>
    ScaleDecoderStream &operator>>(T &v) {
      // check bool
      if constexpr (std::is_same<I, bool>::value) {
        v = decodeBool();
        return *this;
      }
      // check byte
      if constexpr (sizeof(T) == 1u) {
        v = nextByte();
        return *this;
      }
      // decode any other integer
      v = detail::decodeInteger<I>(*this);
      return *this;
    }

    /**
     * @brief scale-decodes any optional value
     * @tparam T type of optional value
     * @param v optional value reference
     * @return reference to stream
     */
    template <class T>
    ScaleDecoderStream &operator>>(boost::optional<T> &v) {
      using mutableT = std::remove_const_t<T>;

      static_assert(std::is_default_constructible_v<mutableT>);

      // optional bool is special case of optional values
      // it is encoded as one byte instead of two
      // as described in specification
      if constexpr (std::is_same<mutableT, bool>::value) {
        v = decodeOptionalBool();
        return *this;
      }
      // detect if optional has value
      bool has_value = false;
      *this >> has_value;
      if (!has_value) {
        v.reset();
        return *this;
      }
      // decode value
      v.emplace();
      return *this >> const_cast<mutableT &>(*v);  // NOLINT
    }

    /**
     * @brief scale-decodes compact integer value
     * @param v compact integer reference
     * @return
     */
    ScaleDecoderStream &operator>>(CompactInteger &v);

    /**
     * @brief decodes vector of items
     * @tparam T item type
     * @param v reference to vector
     * @return reference to stream
     */
    template <class T,
              std::enable_if_t<not std::is_integral_v<T> or sizeof(T) != 1> * =
                  nullptr>
    ScaleDecoderStream &operator>>(std::vector<T> &v) {
      using mutableT = std::remove_const_t<T>;
      using size_type = typename std::list<T>::size_type;

      static_assert(std::is_default_constructible_v<mutableT>);

      CompactInteger size{0u};
      *this >> size;

      auto item_count = size.convert_to<size_type>();

      std::vector<mutableT> vec;
      try {
        vec.resize(item_count);
      } catch (const std::bad_alloc &) {
        common::raise(DecodeError::TOO_MANY_ITEMS);
      }

      for (size_type i = 0u; i < item_count; ++i) {
        *this >> vec[i];
      }

      v = std::move(vec);
      return *this;
    }

    template <
        class T,
        std::enable_if_t<std::is_integral_v<T> and sizeof(T) == 1> * = nullptr>
    ScaleDecoderStream &operator>>(std::vector<T> &v) {
      CompactInteger size{0u};
      *this >> size;

      auto item_count = size.convert_to<size_t>();

      if (not hasMore(item_count)) {
        common::raise(DecodeError::NOT_ENOUGH_DATA);
      }

      try {
        v.reserve(item_count);
        v.insert(
            v.begin(), current_iterator_, current_iterator_ + item_count);
        current_index_ += item_count;
        current_iterator_ += item_count;
      } catch (const std::bad_alloc &) {
        common::raise(DecodeError::TOO_MANY_ITEMS);
      }

      return *this;
    }

    /**
     * @brief decodes array of items
     * @tparam T item type
     * @tparam size of the array
     * @param a reference to the array
     * @return reference to stream
     */
    template <class T, size_t size>
    ScaleDecoderStream &operator>>(std::array<T, size> &a) {
      using mutableT = std::remove_const_t<T>;
      for (size_t i = 0u; i < size; ++i) {
        *this >> const_cast<mutableT &>(a[i]);  // NOLINT
      }
      return *this;
    }

    /**
     * @brief decodes uint256_t from stream
     * @param i value to decode
     * @return reference to stream
     */
    ScaleDecoderStream &operator>>(boost::multiprecision::uint256_t &i) {
      // TODO(akvinikym) PRE-285: maybe move to another file and implement it
      return *this;
    }

    /**
     * @brief decodes string from stream
     * @param v value to decode
     * @return reference to stream
     */
    ScaleDecoderStream &operator>>(std::string &v);

    /**
     * @brief hasMore Checks whether n more bytes are available
     * @param n Number of bytes to check
     * @return True if n more bytes are available and false otherwise
     */
    bool hasMore(uint64_t n) const;

    /**
     * @brief takes one byte from stream and
     * advances current byte iterator by one
     * @return current byte
     */
    uint8_t nextByte();

    using ByteSpan = gsl::span<const uint8_t>;
    using SpanIterator = ByteSpan::const_iterator;
    using SizeType = ByteSpan::size_type;

    ByteSpan span() const {
      return span_;
    }
    SizeType currentIndex() const {
      return current_index_;
    }

   private:
    bool decodeBool();
    /**
     * @brief special case of optional values as described in specification
     * @return boost::optional<bool> value
     */
    boost::optional<bool> decodeOptionalBool();

    template <size_t I, class... Ts>
    void decodeElementOfTuple(std::tuple<Ts...> &v) {
      using T = std::remove_const_t<std::tuple_element_t<I, std::tuple<Ts...>>>;
      *this >> const_cast<T &>(std::get<I>(v));  // NOLINT
      if constexpr (sizeof...(Ts) > I + 1) {
        decodeElementOfTuple<I + 1>(v);
      }
    }

    template <size_t I, class... Ts>
    void tryDecodeAsOneOfVariant(boost::variant<Ts...> &v, size_t i) {
      using T = std::remove_const_t<std::tuple_element_t<I, std::tuple<Ts...>>>;
      static_assert(std::is_default_constructible_v<T>);
      if (I == i) {
        T val;
        *this >> val;
        v = std::forward<T>(val);
        return;
      }
      if constexpr (sizeof...(Ts) > I + 1) {
        tryDecodeAsOneOfVariant<I + 1>(v, i);
      }
    }

    ByteSpan span_;
    SpanIterator current_iterator_;
    SizeType current_index_;
  };

}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_SCALE_DECODER_STREAM_HPP
