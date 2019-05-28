/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_SCALE_ENCODER_STREAM_HPP
#define KAGOME_CORE_SCALE_SCALE_ENCODER_STREAM_HPP

#include <deque>
#include <optional>

#include "common/blob.hpp"
#include "common/byte_stream.hpp"
#include "common/type_traits.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/util.hpp"
#include "scale/variant.hpp"

namespace kagome::scale {
  /**
   * @class ScaleEncoderStream designed to scale-encode data to stream
   */
  class ScaleEncoderStream {
    using Buffer = common::Buffer;

   public:
    /// Getters
    /**
     * @return buffer containing encoded data
     */
    Buffer getBuffer() const;
    /**
     * @brief scale-encodes pair of values
     * @tparam F first value type
     * @tparam S second value type
     * @param p pair of values to encode
     * @return reference to stream
     */

    /// Appenders
    /**
     * @brief puts a byte to buffer
     * @param v byte value
     * @return reference to stream
     */
    ScaleEncoderStream &putByte(uint8_t v);
    /**
     * @brief appends buffer content to stream
     * @param buffer to append
     * @return reference to stream
     */
    ScaleEncoderStream &putBuffer(const Buffer &buffer);
    /**
     * @brief appends vector content to stream
     * @param v vector to append
     * @return reference to stream
     */
    ScaleEncoderStream &put(const std::vector<uint8_t> &v);
    /**
     * @brief copies content between begin and end iterators to the end of
     * stream
     * @tparam It iterator type
     * @param begin first iterator
     * @param end last iterator
     * @return reference to stream
     */

    /// Encoders
    /**
     * @brief appends bytes to stream without encoding them
     * @tparam It iterator over collection of bytes
     * @param begin iterator pointing to the begin of collection
     * @param end iterator pointing to the end of collection
     * @return reference to stream
     */
    template <class It>
    ScaleEncoderStream &put(It &&begin, It &&end) {
      stream_.insert(stream_.end(), begin, end);
      return *this;
    }

    /**
     * @brief scale-encodes pair of objects
     * @tparam F first object type
     * @tparam S second object type
     * @param p pair to encode
     * @return reference to stream
     */
    template <class F, class S>
    ScaleEncoderStream &operator<<(const std::pair<F, S> &p) {
      *this << p.first << p.second;
      return *this;
    }
    /**
     * @brief scale-encodes variant value
     * @tparam T type list
     * @param v value to encode
     * @return reference to stream
     */
    template <class... T>
    ScaleEncoderStream &operator<<(const boost::variant<T...> &v) {
      kagome::scale::variant::encodeVariant(v, *this);
      return *this;
    }
    /**
     * @brief scale-encodes collection of same time items
     * @tparam T type of item
     * @param c collection to encode
     * @return reference to stream
     */
    template <class T>
    ScaleEncoderStream &operator<<(const std::vector<T> &c) {
      return encodeCollection(c.size(), c.begin(), c.end());
    }
    /**
     * @brief scale-encodes optional value
     * @tparam T value type
     * @param v value to encode
     * @return reference to stream
     */
    template <class T>
    ScaleEncoderStream &operator<<(const std::optional<T> &v) {
      if (!v.has_value()) {
        return putByte(0u);
      }
      return putByte(1u) << *v;
    }
    /**
     * @brief scale-encodes std::array as sequence of bytes, not collection
     * @tparam T array item type
     * @tparam size array size
     * @param v value to encode
     * @return reference to stream
     */
    template <class T, size_t size>
    ScaleEncoderStream &operator<<(const std::array<T, size> &v) {
      return put(v.begin(), v.end());
    }
    /**
     * @brief scale-encodes common::Blob as sequence of bytes, not collection
     * @tparam size size of blob
     * @param v blob to encode
     * @return reference to stream
     */
    template <size_t size>
    ScaleEncoderStream &operator<<(const common::Blob<size> &v) {
      return put(v.begin(), v.end());
    }
    /**
     * @brief scale-encodes std::reference_wrapper of a type
     * @tparam T underlying type
     * @param v value to encode
     * @return reference to stream;
     */
    template <class T>
    ScaleEncoderStream &operator<<(const std::reference_wrapper<T> &v) {
      return *this << static_cast<const T&>(v);
    }
    /**
     * @brief scale-encodes common::Buffer as collection of bytes
     * @param v buffer to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(const Buffer &v) {
      return encodeCollection(v.size(), v.begin(), v.end());
    }

    /**
     * @brief scale-encodes a string view
     * @param sv string_view item
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(std::string_view sv) {
      return encodeCollection(sv.size(), sv.begin(), sv.end());
    }

    /**
     * @brief scale-encodes any integral type including bool
     * @tparam T integral type
     * @param v value of integral type
     * @return reference to stream
     */
    template <typename T, typename I = common::remove_cv_ref_t<T>,
              typename = std::enable_if_t<std::is_integral<I>::value>>
    ScaleEncoderStream &operator<<(T &&v) {
      // encode bool
      if constexpr (std::is_same<I, bool>::value) {
        uint8_t byte = (v ? 0x01u : 0x00u);
        return putByte(byte);
      }
      // encode any other integer
      impl::encodeInteger<I>(v, *this);
      return *this;
    }

    /**
     * @brief scale-encodes BigInteger value as compact integer
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(const BigInteger &v);
    /**
     * scale-encodes tribool value
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(tribool v);

    /**
     * @brief scale-encodes any collection
     * @tparam It iterator over collection of bytes
     * @param size size of the collection
     * @param begin iterator pointing to the begin of collection
     * @param end iterator pointing to the end of collection
     * @return reference to stream
     */
    template <class It>
    ScaleEncoderStream &encodeCollection(const BigInteger &size, It &&begin,
                                         It &&end) {
      *this << size;
      for (auto &&it = begin; it != end; ++it) {  // NOLINT
        *this << *it;
      }
      return *this;
    }

   private:
    std::deque<uint8_t> stream_;
  };

}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_SCALE_ENCODER_STREAM_HPP
