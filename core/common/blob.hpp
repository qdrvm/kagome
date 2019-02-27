/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOB_HPP
#define KAGOME_BLOB_HPP

#include <boost/format.hpp>
#include "common/hexutil.hpp"
#include "common/result.hpp"

namespace kagome::common {

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
      return hex(this->data(), size_);
    }

    /**
     * Create Blob from arbitrary string, putting its bytes into the blob
     * @param data arbitrary string containing
     * @return result containing Blob object if string has proper size,
     * otherwise result contains string with error message
     */
    static expected::Result<Blob<size_>, std::string> fromString(
        std::string_view data) {
      if (data.size() != size_) {
        std::string value = "blob_t: input string has incorrect length. Found: "
            + std::to_string(data.size())
            + +", required: " + std::to_string(size_);
        return expected::Error{value};
      }

      Blob<size_> b;
      std::copy(data.begin(), data.end(), b.begin());

      return expected::Value{b};
    }

    /**
     * Create Blob from hex string
     * @param hex hex string
     * @return result containing Blob object if hex string has proper size and
     * is in hex format, otherwise result contains error message
     */
    static expected::Result<Blob<size_>, std::string> fromHex(
        std::string_view hex) {
      return unhex(hex) | [&hex](const std::vector<uint8_t> &bytes)
                 -> expected::Result<Blob<size_>, std::string> {
        if (bytes.size() != size_) {
          const static std::string error_message_template =
              "Provided hex string %1 has decoded bytes size %2. Expected "
              "bytes size: %3";
          auto formatted_error = boost::format(error_message_template) % hex
              % bytes.size() % size_;
          return expected::Error{formatted_error.str()};
        }
        Blob<size_> blob;
        std::copy(bytes.begin(), bytes.end(), blob.begin());
        return expected::Value{blob};
      };
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

}  // namespace kagome::common

#endif  // KAGOME_BLOB_HPP
