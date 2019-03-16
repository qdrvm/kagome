/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_ADVANCED_HPP
#define KAGOME_SCALE_ADVANCED_HPP

#include "common/scale/compact.hpp"
#include "common/scale/fixedwidth.hpp"
#include "common/scale/type_decoder.hpp"
#include "common/scale/type_encoder.hpp"

namespace kagome::common::scale::collection {
  /**
   *  @brief encodeCollection function scale-encodes
   *         collection of same type values
   *  @tparam T Item type stored in collection
   *  @param collection source vector items
   *  @return byte array containing encoded collection
   */
  template <class T>
  EncodeResult encodeCollection(const std::vector<T> &collection) {
    auto headerResult = compact::encodeInteger(BigInteger{collection.size()});
    if (!isSucceeded(headerResult)) {
      return EncodeResult::ErrorType{EncodeError::kEncodeHeaderError};
    }

    ByteArray result;
    headerResult.match(
        [&collection, &result](const expected::Value<ByteArray> &v) {
          if (std::is_integral<T>()) {
            size_t bytes_required = v.value.size();
            bytes_required += collection.size() * sizeof(T);
            result.reserve(bytes_required);
          }

          result.insert(result.end(), v.value.begin(), v.value.end());
        },
        [](const expected::Error<EncodeError> &e) {});

    TypeEncoder<T> encoder{};
    for (size_t i = 0; i < collection.size(); ++i) {
      bool is_succeeded = true;
      encoder.encode(collection[i])
          .match(
              [&result](const expected::Value<ByteArray> &v) {
                result.insert(result.end(), v.value.begin(), v.value.end());
              },
              [&is_succeeded](const expected::Error<EncodeError> &) {
                is_succeeded = false;
              });

      if (!is_succeeded) {
        return expected::Error{EncodeError::kFailed};
      }
    }

    return expected::Value{result};
  }

  /**
   * @brief DecodeCollectionResult is result of decodeCollection operation
   */
  template <class T>
  using DecodeCollectionResult = expected::Result<std::vector<T>, DecodeError>;

  /**
   * @brief decodeCollection function decodes collection containing items of same specified type
   * @tparam T collection item type
   * @param stream source stream
   * @return decoded collection or error
   */
  template <class T>
  DecodeCollectionResult<T> decodeCollection(Stream &stream) {
    // determine number of items
    auto headerResult = compact::decodeInteger(stream);
    if (!isSucceeded(headerResult)) {
      return expected::Error{DecodeError::kInvalidData};
    }

    BigInteger number;
    headerResult.match(
        [&number](const expected::Value<BigInteger> &v) { number = v.value; },
        [](const auto &) {});

    if (number > std::numeric_limits<std::vector<uint8_t>::size_type>::max()) {
      return expected::Error{DecodeError::kTooManyItems};
    }

    auto number_64 = number.convert_to<uint64_t>();

    BigInteger required_bytes = number * sizeof(T);
    if (required_bytes > std::numeric_limits<uint64_t>::max()) {
      return expected::Error{DecodeError::kInvalidData};
    }

    if (!stream.hasMore(required_bytes.convert_to<uint64_t>())) {
      return expected::Error<DecodeError>{DecodeError::kNotEnoughData};
    }

    std::vector<T> resultVector;
    resultVector.reserve(number.convert_to<size_t>());

    TypeDecoder<T> type_decoder{};
    // parse items one by one
    for (uint64_t i = 0; i < number_64; ++i) {
      auto r = type_decoder.decode(stream);
      if (!isSucceeded(r)) {
        return expected::Error{DecodeError::kInvalidData};
      }

      r.match(
          [&resultVector](const expected::Value<T> &v) {
            resultVector.push_back(v.value);
          },
          [](const expected::Error<DecodeError> &) {});
    }

    return expected::Value{resultVector};
  }
}  // namespace kagome::common::scale::collection

#endif  // KAGOME_SCALE_ADVANCED_HPP
