/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_TYPES_HPP
#define KAGOME_SCALE_TYPES_HPP

#include <cstdint>
#include <vector>

#include <boost/logic/tribool.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "common/result.hpp"

namespace kagome::common::scale {
  using ByteArray = std::vector<uint8_t>;
  using BigInteger = boost::multiprecision::cpp_int;

  using tribool = boost::logic::tribool;

  constexpr auto indeterminate = boost::logic::indeterminate;
  constexpr auto isIndeterminate = [](tribool value) {
    return boost::logic::indeterminate(value);
  };

  /**
   * @brief interface for Decoders
   * it is required for nesting decoders
   */
  class Stream {
   public:
    virtual ~Stream() = default;

    /**
     * @brief hasMore Checks whether n more bytes are available
     * @param n Number of bytes to check
     * @return True if n more bytes are available and false otherwise
     */
    virtual bool hasMore(uint64_t n) const = 0;

    /**
     * @brief nextByte Takes current byte and moves pointer to the next one
     * @return Current byte
     */
    virtual std::optional<uint8_t> nextByte() = 0;
  };

  /**
   * @brief DecoderError enum provides codes of errors for Decoder methods
   */
  enum class DecodeError : size_t {
    kNotEnoughData,    ///< not enough data to decode value
    kUnexpectedValue,  ///< unexpected value
    kInvalidData,      ///< invalid data
    kValueIsTooBig,    ///< cannot handle value, because it is too big,
                       ///< for example number of bytes in collection
    kTooManyItems      ///< too many items

  };

  /**
   * @brief EncodeError enum provides codes of errors for Encoder methods
   */
  enum class EncodeError : size_t {
    kSuccess,                ///< no error, operation succeeded
    kInvalidItem,            ///< item is invalid
    kNegativeCompactNumber,  ///< negative BigInteger cannot be encoded
    kWrongCategory,          ///< wrong encoding category
    kValueIsTooBig,          ///< big integer is out of range
    kTooManyItems,           ///< too many item in collection, cannot encode
    kEncodeHeaderError,      ///< failed to encode header
    kFailed                  ///< failed for unspecified reason
  };

  /**
   * @brief TypeEncodeResult is result of encode operation
   */
  using EncodeResult = EncodeError;

  /**
   * @brief TypeDecodeResult is result of decode operation
   */
  template <class T>
  using TypeDecodeResult = expected::Result<T, DecodeError>;
}  // namespace kagome::common::scale

#endif  // KAGOME_SCALE_TYPES_HPP
