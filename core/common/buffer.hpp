/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_BUFFER
#define KAGOME_COMMON_BUFFER

#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <boost/container_hash/hash.hpp>
#include <boost/operators.hpp>
#include <gsl/span>

#include "common/buffer_view.hpp"
#include "common/size_limited_containers.hpp"
#include "hexutil.hpp"
#include "macro/endianness_utils.hpp"
#include "outcome/outcome.hpp"

namespace kagome::common {

  /**
   * @brief Class represents arbitrary (including empty) byte buffer.
   */
  template <size_t MaxSize>
  class SLBuffer : public SLVector<uint8_t, MaxSize> {
   public:
    using Base = SLVector<uint8_t, MaxSize>;

    template <size_t OtherMaxSize>
    using OtherMaxSizeBase = SLVector<uint8_t, OtherMaxSize>;

    SLBuffer() = default;

    /**
     * @brief lvalue construct buffer from a byte vector
     */
    explicit SLBuffer(const typename Base::Base &other) : Base(other) {}
    explicit SLBuffer(typename Base::Base &&other) : Base(std::move(other)) {}

    template <size_t OtherMaxSize>
    SLBuffer(const OtherMaxSizeBase<OtherMaxSize> &other) : Base(other) {}

    template <size_t OtherMaxSize>
    SLBuffer(OtherMaxSizeBase<OtherMaxSize> &&other) : Base(std::move(other)) {}

    SLBuffer(const gsl::span<const typename Base::value_type> &s)
        : Base(s.begin(), s.end()) {}

    SLBuffer(const uint8_t *begin, const uint8_t *end) : Base(begin, end){};

    using Base::Base;
    using Base::operator=;

    SLBuffer &reserve(size_t size) {
      Base::reserve(size);
      return *this;
    }

    SLBuffer &resize(size_t size) {
      Base::resize(size);
      return *this;
    }

    template <size_t OtherMaxSize>
    SLBuffer &operator+=(const SLBuffer<OtherMaxSize> &other) noexcept {
      return putBuffer(other);
    }

    /**
     * @brief Put a 8-bit {@param n} in this buffer.
     * @return this buffer, suitable for chaining.
     */
    SLBuffer &putUint8(uint8_t n) {
      SLBuffer::push_back(n);
      return *this;
    }

    /**
     * @brief Put a 32-bit {@param n} number in this buffer. Will be serialized
     * as big-endian number.
     * @return this buffer, suitable for chaining.
     */
    SLBuffer &putUint32(uint32_t n) {
      n = htobe32(n);
      const auto *begin = reinterpret_cast<uint8_t *>(&n);
      const auto *end = begin + sizeof(n) / sizeof(uint8_t);
      Base::insert(this->end(), std::move(begin), std::move(end));
      return *this;
    }

    /**
     * @brief Put a 64-bit {@param n} number in this buffer. Will be serialized
     * as big-endian number.
     * @return this buffer, suitable for chaining.
     */
    SLBuffer &putUint64(uint64_t n) {
      n = htobe64(n);
      const auto *begin = reinterpret_cast<uint8_t *>(&n);
      const auto *end = begin + sizeof(n) / sizeof(uint8_t);
      Base::insert(this->end(), std::move(begin), std::move(end));
      return *this;
    }

    /**
     * @brief Put a string into byte buffer
     * @param s arbitrary string
     * @return this buffer, suitable for chaining.
     */
    SLBuffer &put(std::string_view range) {
      Base::insert(Base::end(), range.begin(), range.end());
      return *this;
    }

    /**
     * @brief Put a vector of bytes into byte buffer
     * @param s arbitrary vector of bytes
     * @return this buffer, suitable for chaining.
     */
    SLBuffer &put(const std::vector<uint8_t> &range) {
      Base::insert(Base::end(), range.begin(), range.end());
      return *this;
    }

    /**
     * @brief Put a sequence of bytes into byte buffer
     * @param s arbitrary span of bytes
     * @return this buffer, suitable for chaining.
     */
    SLBuffer &put(gsl::span<const uint8_t> range) {
      Base::insert(Base::end(), range.begin(), range.end());
      return *this;
    }

    /**
     * @brief Put a array of bytes bounded by pointers into byte buffer
     * @param begin pointer to the array start
     *        end pointer to the address after the last element
     * @return this buffer, suitable for chaining.
     */
    SLBuffer &putBytes(const uint8_t *begin, const uint8_t *end) {
      return putRange(begin, end);
    }

    /**
     * @brief Put another buffer content at the end of current one
     * @param buf another buffer
     * @return this buffer suitable for chaining.
     */
    template <size_t OtherMaxSize2>
    SLBuffer &putBuffer(const SLBuffer<OtherMaxSize2> &range) {
      Base::insert(Base::end(), range.begin(), range.end());
      return *this;
    }

    /**
     * @brief getter for vector of bytes
     */
    const std::vector<uint8_t> &asVector() const {
      return static_cast<const std::vector<uint8_t> &>(*this);
    }
    std::vector<uint8_t> &asVector() {
      return static_cast<std::vector<uint8_t> &>(*this);
    }

    std::vector<uint8_t> toVector() & {
      return static_cast<std::vector<uint8_t> &>(*this);
    }

    std::vector<uint8_t> toVector() && {
      return std::move(static_cast<std::vector<uint8_t> &>(*this));
    }

    /**
     * Returns a copy of a part of the buffer
     * Works alike subspan() of gsl::span
     */
    SLBuffer subbuffer(size_t offset = 0, size_t length = -1) const {
      return SLBuffer(gsl::make_span(*this).subspan(offset, length));
    }

    BufferView view(size_t offset = 0, size_t length = -1) const {
      return BufferView{gsl::make_span(*this).subspan(offset, length)};
    }

    /**
     * @brief encode bytearray as hex
     * @return hex-encoded string
     */
    std::string toHex() const {
      return hex_lower(*this);
    }

    /**
     * @brief Construct SLBuffer from hex string
     * @param hex hex-encoded string
     * @return result containing constructed buffer if input string is
     * hex-encoded string.
     */
    static outcome::result<SLBuffer> fromHex(std::string_view hex) {
      OUTCOME_TRY(bytes, unhex(hex));
      return outcome::success(SLBuffer(std::move(bytes)));
    }

    /**
     * @brief return content of bytearray as string
     * @note Does not ensure correct encoding
     * @return string
     */
    std::string toString() const {
      return std::string{Base::cbegin(), Base::cend()};
    }

    /**
     * @brief return content of bytearray as a string copy data
     * @note Does not ensure correct encoding
     * @return string
     */
    std::string_view asString() const {
      return std::string_view(reinterpret_cast<const char *>(Base::data()),
                              Base::size());  // NOLINT
    }

    /**
     * @brief stores content of a string to byte array
     */
    static SLBuffer fromString(const std::string_view &src) {
      return {src.begin(), src.end()};
    }

   private:
    template <
        typename OutIt,
        typename InIt,
        bool isIter = std::is_same_v<OutIt, typename Base::iterator>,
        bool isConstIter = std::is_same_v<OutIt, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>,
        typename = std::enable_if_t<std::is_base_of_v<
            std::input_iterator_tag,
            typename std::iterator_traits<InIt>::iterator_category>>>
    SLBuffer &putRange(const InIt &begin, const InIt &end) {
      static_assert(sizeof(*begin) == 1);
      insert(this->end(), begin, end);
      return *this;
    }
  };

  template <size_t MaxSize>
  inline std::ostream &operator<<(std::ostream &os,
                                  const SLBuffer<MaxSize> &buffer) {
    return os << BufferView(buffer);
  }

  //  using Buffer = SLBuffer<std::numeric_limits<size_t>::max()>;
  typedef SLBuffer<std::numeric_limits<size_t>::max()> Buffer;

  static inline const Buffer kEmptyBuffer{};

  using BufferMutRef = std::reference_wrapper<Buffer>;
  using BufferConstRef = std::reference_wrapper<const Buffer>;

  namespace literals {
    /// creates a buffer filled with characters from the original string
    /// mind that it does not perform unhexing, there is ""_unhex for it
    inline Buffer operator""_buf(const char *c, size_t s) {
      std::vector<uint8_t> chars(c, c + s);
      return Buffer(std::move(chars));
    }

    // TODO(GaroRobe): After migrating to C++20 would be good to use
    //  literal operator template
    inline Buffer operator""_hex2buf(const char *c, size_t s) {
      return Buffer::fromHex({c, s}).value();
    }
  }  // namespace literals

}  // namespace kagome::common

template <size_t N>
struct std::hash<kagome::common::SLBuffer<N>> {
  size_t operator()(const kagome::common::SLBuffer<N> &x) const {
    return boost::hash_range(x.begin(), x.end());
  }
};

template <>
struct fmt::formatter<kagome::common::Buffer>
    : fmt::formatter<kagome::common::BufferView> {};

#endif  // KAGOME_COMMON_BUFFER
