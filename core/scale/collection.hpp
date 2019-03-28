/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_ADVANCED_HPP
#define KAGOME_SCALE_ADVANCED_HPP

#include <gsl/span>

#include "common/buffer.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/type_decoder.hpp"
#include "scale/type_encoder.hpp"

namespace kagome::common::scale::collection {
  /**
   *  @brief encodeCollection function scale-encodes
   *         collection of same type values
   *  @tparam T Item type stored in collection
   *  @param collection source vector items
   *  @return true if operation succeeded and false otherwise
   */
  template <class T>
  bool encodeCollection(const std::vector<T> &collection, Buffer &out) {
    Buffer encoded_collection;
    auto header_result =
        compact::encodeInteger(collection.size(), encoded_collection);
    if (!header_result) {
      return false;
    }

    TypeEncoder<T> encoder{};
    for (size_t i = 0; i < collection.size(); ++i) {
      auto encode_result = encoder.encode(collection[i], encoded_collection);
      if (!encode_result) {
        return false;
      }
    }

    out.put(encoded_collection.toVector());

    return true;
  }  // namespace kagome::common::scale::collection


  /**
   * @brief encodes buffer as collection of bytes
   * @param buf bytes to encode
   * @param out output stream
   * @return true if operation succeeded and false otherwise
   */
  bool encodeBuffer(const Buffer & buf, Buffer &out);

  /**
   * @brief DecodeCollectionResult is result of decodeCollection operation
   */
  template <class T>
  using DecodeCollectionResult = expected::Result<std::vector<T>, DecodeError>;

//  DecodeCollectionResult<uint8_t> decodeBytes()

  /**
   * @brief decodeCollection function decodes collection containing items of
   * same specified type
   * @tparam T collection item type
   * @param stream source stream
   * @return decoded collection or error
   */
  template <class T>
  DecodeCollectionResult<T> decodeCollection(Stream &stream) {
    // determine number of items
    auto header_result = compact::decodeInteger(stream);
    if (header_result.hasError()) {
      return expected::Error{header_result.getError()};
    }

    BigInteger item_count_big = header_result.getValue();
    if (item_count_big
        > std::numeric_limits<std::vector<uint8_t>::size_type>::max()) {
      return expected::Error{DecodeError::kTooManyItems};
    }

    auto item_count = item_count_big.convert_to<uint64_t>();

    BigInteger required_bytes = item_count_big * sizeof(T);
    if (required_bytes > std::numeric_limits<uint64_t>::max()) {
      return expected::Error{DecodeError::kTooManyItems};
    }

    if (!stream.hasMore(required_bytes.convert_to<uint64_t>())) {
      return expected::Error<DecodeError>{DecodeError::kNotEnoughData};
    }

    std::vector<T> decoded_collection;
    decoded_collection.reserve(item_count);

    // parse items one by one
    TypeDecoder<T> type_decoder{};
    for (uint64_t i = 0; i < item_count; ++i) {
      auto r = type_decoder.decode(stream);
      if (r.hasError()) {
        return expected::Error{r.getError()};
      }

      decoded_collection.push_back(r.getValue());
    }

    return expected::Value{decoded_collection};
  }
}  // namespace kagome::common::scale::collection

#endif  // KAGOME_SCALE_ADVANCED_HPP
