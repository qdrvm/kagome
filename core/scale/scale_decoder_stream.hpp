/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_SCALE_DECODER_STREAM_HPP
#define KAGOME_CORE_SCALE_SCALE_DECODER_STREAM_HPP

#include <optional>

#include <gsl/span>
#include "scale/detail/fixed_witdh_integer.hpp"
#include "scale/detail/variant.hpp"
#include "scale/outcome_throw.hpp"

namespace kagome::scale {
  class ScaleDecoderStream {
   public:
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
      return *this >> p.first >> p.second;
    }

    /**
     * @brief scale-decodes variant value
     * @tparam T type list
     * @param v value to decode
     * @return reference to stream
     */
    template <class... T>
    ScaleDecoderStream &operator>>(boost::variant<T...> &v) {
      return detail::decodeVariant(*this, v);
    }

    /**
     * @brief scale-encodes any integral type including bool
     * @tparam T integral type
     * @param v value of integral type
     * @return reference to stream
     */
    template <typename T, typename I = std::decay_t<T>,
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
      // TODO(yuraz): PRE-119 refactor decodeInteger
      auto &&res = detail::decodeInteger<I>(*this);
      return *this;
    }

    /**
     * @brief scale-decodes any optional value
     * @tparam T type of optional value
     * @param v optional value reference
     * @return reference to stream
     */
    template <class T>
    ScaleDecoderStream &operator>>(std::optional<T> &v) {
      bool has_value = false;
      *this >> has_value;
      if (!has_value) {
        v = std::nullopt;
        return *this;
      }
      T t{};
      *this >> t;
      v = t;

      return *this;
    }

    /**
     * @brief scale-decodes compact integer value
     * @param v compact integer reference
     * @return
     */
    ScaleDecoderStream &operator>>(BigInteger &v);

    /**
     * @brief decodes collection of items
     * @tparam T item type
     * @param v reference to collection
     * @return reference to stream
     */
    template <class T>
    ScaleDecoderStream &operator>>(std::vector<T> &v) {
      v.clear();
      BigInteger size{0u};
      *this >> size;

      using size_type = typename std::vector<T>::size_type;

      if (size > std::numeric_limits<size_type>::max()) {
        common::raise(DecodeError::TOO_MANY_ITEMS);
      }
      auto item_count = size.convert_to<size_type>();
      v.reserve(item_count);
      for (size_type i = 0u; i < item_count; ++i) {
        T t{};
        *this >> t;
        v.push_back(std::move(t));
      }
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
    bool hasMore(uint64_t n) const {
      return current_ptr_ + n <= end_ptr_;
    }

    /**
     * @brief takes one byte from stream and
     * advances current byte iterator by one
     * @return current byte
     */
    uint8_t nextByte();

    /**
     * @brief advances current byte iterator by specified number of positions
     * @param dist number of positions to advance iterator
     * @return nothing, can throw DecodeError::OUT_OF_BOUNDARIES exception
     */
    void advance(uint64_t dist);

   private:
    bool decodeBool();

    using ByteSpan = gsl::span<const uint8_t>;
    using SpanIterator = ByteSpan::const_iterator;

    SpanIterator current_ptr_;
    SpanIterator end_ptr_;
  };

}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_SCALE_DECODER_STREAM_HPP
