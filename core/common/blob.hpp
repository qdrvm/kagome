/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOB_HPP
#define KAGOME_BLOB_HPP

#include <array>

#include <boost/format.hpp>
#include "common/hexutil.hpp"

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
    /**
     * Initialize blob value
     */
    Blob() {
      this->fill(0);
    }

    /**
     * @brief constructor enabling initializer list
     * @param l initializer list
     */
    Blob(const std::array<byte_t, size_> &l) {
      std::copy(l.begin(), l.end(), this->begin());
    }

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
      return hex_upper({this->begin(), this->end()});
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
      if (res.size() != size_) {
        return BlobError::INCORRECT_LENGTH;
      }

      Blob<size_> blob;
      std::copy(res.begin(), res.end(), blob.begin());
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
  template <class Stream, size_t size>
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
  template<class Stream,size_t size>
  Stream &operator>>(Stream &s, Blob<size> &blob) {
    for (auto &&it = blob.begin(), end = blob.end(); it != end; ++it) {
      s >> *it;
    }
    return s;
  }
}  // namespace kagome::common

OUTCOME_HPP_DECLARE_ERROR(kagome::common, BlobError);

#endif  // KAGOME_BLOB_HPP
