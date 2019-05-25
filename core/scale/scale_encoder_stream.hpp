/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_SCALE_ENCODER_STREAM_HPP
#define KAGOME_CORE_SCALE_SCALE_ENCODER_STREAM_HPP

#include <deque>
#include <optional>

#include "common/byte_stream.hpp"
#include "common/blob.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
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
     * @brief copies content between begin and end iterators to the end of stream
     * @tparam It iterator type
     * @param begin first iterator
     * @param end last iterator
     * @return reference to stream
     */

    /// Encoders
    template<class It>
    ScaleEncoderStream &put(It begin, It end) {
      stream_.insert(stream_.end(), begin, end);
      return *this;
    }
    /**
     * @brief scale-encodes pair of values
     * @tparam F first value type
     * @tparam S second value type
     * @param p pair of values to encode
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
    ScaleEncoderStream &operator<<(const boost::variant<T...> v) {
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
      *this << BigInteger(c.size());
      for (auto &&item : c) {
        *this << item;
      }
      return *this;
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
      put(v.begin(), v.end());
      return *this;
    }
    /**
     * @brief scale-encodes common::Blob as sequence of bytes, not collection
     * @tparam size size of blob
     * @param v blob to encode
     * @return reference to stream
     */
    template <size_t size>
    ScaleEncoderStream &operator<<(const common::Blob<size> & v) {
      put(v.begin(), v.end());
      return *this;
    }
    /**
     * @brief scale-encodes common::Buffer as collection of bytes
     * @param v buffer to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(const Buffer &v) {
      return *this << v.toVector();
    }

    ScaleEncoderStream &operator<<(std::string_view sv) {
      *this << BigInteger(sv.size());
      return put(sv.begin(), sv.end());
    }

    /**
     * @brief scale-encodes uint8_t value as fixed size integer
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(uint8_t v);
    /**
     * @brief scale-encodes int8_t value as fixed size integer
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(int8_t v);
    /**
     * @brief scale-encodes uint16_t value as fixed size integer
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(uint16_t v);
    /**
     * @brief scale-encodes int16_t value as fixed size integer
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(int16_t v);
    /**
     * @brief scale-encodes uint32_t value as fixed size integer
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(uint32_t v);
    /**
     * @brief scale-encodes int32_t value as fixed size integer
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(int32_t v);
    /**
     * @brief scale-encodes uint64_t value as fixed size integer
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(uint64_t v);
    /**
     * @brief scale-encodes int64_t value as fixed size integer
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(int64_t v);
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
     * scale-encodes bool value
     * @param v value to encode
     * @return reference to stream
     */
    ScaleEncoderStream &operator<<(bool v);

    // TODO(yuraz): PRE-??? add operators for signed long, signed/unsigned: short, int, long long

   private:
    std::deque<uint8_t> stream_;
  };

}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_SCALE_ENCODER_STREAM_HPP
