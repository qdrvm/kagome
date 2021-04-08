/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOB_HPP
#define KAGOME_BLOB_HPP

#include <array>

#include <boost/functional/hash.hpp>

#include "common/buffer.hpp"
#include "common/hexutil.hpp"

#define KAGOME_BLOB_STRICT_TYPEDEF(class_name, blob_size)                    \
  struct class_name : public ::kagome::common::Blob<blob_size> {             \
    class_name() = default;                                                  \
    class_name(const class_name &) = default;                                \
    class_name(class_name &&) = default;                                     \
    class_name &operator=(const class_name &) = default;                     \
    class_name &operator=(class_name &&) = default;                          \
                                                                             \
    explicit class_name(const ::kagome::common::Blob<blob_size> &blob)       \
        : Blob<blob_size>{blob} {}                                           \
    explicit class_name(::kagome::common::Blob<blob_size> &&blob)            \
        : Blob<blob_size>{std::move(blob)} {}                                \
                                                                             \
    ~class_name() = default;                                                 \
                                                                             \
    class_name &operator=(const ::kagome::common::Blob<blob_size> &blob) {   \
      Blob::operator=(blob);                                                 \
      return *this;                                                          \
    }                                                                        \
                                                                             \
    class_name &operator=(::kagome::common::Blob<blob_size> &&blob) {        \
      Blob::operator=(std::move(blob));                                      \
      return *this;                                                          \
    }                                                                        \
                                                                             \
    static ::outcome::result<class_name> fromString(std::string_view data) { \
      OUTCOME_TRY(blob, Blob<blob_size>::fromString(data));                  \
      return class_name{std::move(blob)};                                    \
    }                                                                        \
                                                                             \
    static ::outcome::result<class_name> fromHex(std::string_view hex) {     \
      OUTCOME_TRY(blob, Blob<blob_size>::fromHex(hex));                      \
      return class_name{std::move(blob)};                                    \
    }                                                                        \
                                                                             \
    static ::outcome::result<class_name> fromHexWithPrefix(                  \
        std::string_view hex) {                                              \
      OUTCOME_TRY(blob, Blob<blob_size>::fromHexWithPrefix(hex));            \
      return class_name{std::move(blob)};                                    \
    }                                                                        \
                                                                             \
    static ::outcome::result<class_name> fromSpan(                           \
        const gsl::span<const uint8_t> &span) {                              \
      OUTCOME_TRY(blob, Blob<blob_size>::fromSpan(span));                    \
      return class_name{std::move(blob)};                                    \
    }                                                                        \
  };

namespace kagome::common {

  /**
   * Error codes for exceptions that may occur during blob initialization
   */
  enum class BlobError { INCORRECT_LENGTH = 1 };

  using byte_t = uint8_t;

  /**
   * Base type which represents blob of fixed size.
   *
   * std::string is convenient to use but it is not safe.
   * We can not specify the fixed length for string.
   *
   * For std::array it is possible, so we prefer it over std::string.
   */
  template <size_t size_>
  class Blob : public std::array<byte_t, size_> {
   public:
    using const_narref = const byte_t (&)[size_];
    using const_narptr = const byte_t (*)[size_];
    /**
     * Initialize blob value
     */
    Blob() {
      this->fill(0);
    }

    const_narref internal_array_reference() const {
      return *const_narptr(this->data());
    }

    /**
     * @brief constructor enabling initializer list
     * @param l initializer list
     */
    explicit Blob(const std::array<byte_t, size_> &l) {
      std::copy(l.begin(), l.end(), this->begin());
    }

    virtual ~Blob() = default;

    /**
     * In compile-time returns size of current blob.
     */
    constexpr static size_t size() {
      return size_;
    }

    /**
     * Converts current blob to std::string
     */
    std::string toString() const noexcept {
      return std::string{this->begin(), this->end()};
    }

    /**
     * Converts current blob to hex string.
     */
    std::string toHex() const noexcept {
      return hex_lower({this->begin(), this->end()});
    }

    /**
     * Create Blob from arbitrary string, putting its bytes into the blob
     * @param data arbitrary string containing
     * @return result containing Blob object if string has proper size
     */
    static outcome::result<Blob<size_>> fromString(std::string_view data) {
      if (data.size() != size_) {
        return BlobError::INCORRECT_LENGTH;
      }

      Blob<size_> b;
      std::copy(data.begin(), data.end(), b.begin());

      return b;
    }

    /**
     * Create Blob from hex string
     * @param hex hex string
     * @return result containing Blob object if hex string has proper size and
     * is in hex format
     */
    static outcome::result<Blob<size_>> fromHex(std::string_view hex) {
      OUTCOME_TRY(res, unhex(hex));
      return fromSpan(res);
    }

    /**
     * Create Blob from hex string prefixed with 0x
     * @param hex hex string
     * @return result containing Blob object if hex string has proper size and
     * is in hex format
     */
    static outcome::result<Blob<size_>> fromHexWithPrefix(
        std::string_view hex) {
      OUTCOME_TRY(res, unhexWith0x(hex));
      return fromSpan(res);
    }

    /**
     * Create Blob from span of uint8_t
     * @param buffer
     * @return
     */
    static outcome::result<Blob<size_>> fromSpan(
        const gsl::span<const uint8_t> &span) {
      if (span.size() != size_) {
        return BlobError::INCORRECT_LENGTH;
      }

      Blob<size_> blob;
      std::copy(span.begin(), span.end(), blob.begin());
      return blob;
    }
  };

  // extern specification of the most frequently instantiated blob
  // specializations, used mostly for Hash instantiation
  extern template class Blob<8ul>;
  extern template class Blob<16ul>;
  extern template class Blob<32ul>;
  extern template class Blob<64ul>;

  // Hash specializations
  using Hash64 = Blob<8>;
  using Hash128 = Blob<16>;
  using Hash256 = Blob<32>;
  using Hash512 = Blob<64>;

  /**
   * @brief scale-encodes blob instance to stream
   * @tparam Stream output stream type
   * @tparam size blob size
   * @param s output stream reference
   * @param blob value to encode
   * @return reference to stream
   */
  template <class Stream,
            size_t size,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Blob<size> &blob) {
    for (auto &&it = blob.begin(), end = blob.end(); it != end; ++it) {
      s << *it;
    }
    return s;
  }

  /**
   * @brief decodes blob instance from stream
   * @tparam Stream output stream type
   * @tparam size blob size
   * @param s input stream reference
   * @param blob value to encode
   * @return reference to stream
   */
  template <class Stream,
            size_t size,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Blob<size> &blob) {
    for (auto &&it = blob.begin(), end = blob.end(); it != end; ++it) {
      s >> *it;
    }
    return s;
  }

  template <size_t N>
  inline std::ostream &operator<<(std::ostream &os, const Blob<N> &blob) {
    return os << blob.toHex();
  }

}  // namespace kagome::common

template <size_t N>
struct std::hash<kagome::common::Blob<N>> {
  auto operator()(const kagome::common::Blob<N> &blob) const {
    return boost::hash_range(blob.data(), blob.data() + N);  // NOLINT
  }
};

OUTCOME_HPP_DECLARE_ERROR(kagome::common, BlobError);

#endif  // KAGOME_BLOB_HPP
