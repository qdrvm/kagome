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
  outcome::result<void> encodeCollection(const std::vector<T> &collection,
                                         Buffer &out) {
    Buffer encoded_collection;
    OUTCOME_TRY(compact::encodeInteger(collection.size(), encoded_collection));

    TypeEncoder<T> encoder{};
    for (size_t i = 0; i < collection.size(); ++i) {
      OUTCOME_TRY(encoder.encode(collection[i], encoded_collection));
    }

    out.putBuffer(encoded_collection);

    return outcome::success();
  }  // namespace kagome::common::scale::collection

  /**
   * @brief encodes buffer as collection of bytes
   * @param buf bytes to encode
   * @param out output stream
   * @return true if operation succeeded and false otherwise
   */
  outcome::result<void> encodeBuffer(const Buffer &buf, Buffer &out);

  /**
   * @brief decodeCollection function decodes collection containing items of
   * same specified type
   * @tparam T collection item type
   * @param stream source stream
   * @return decoded collection or error
   */
  template <class T>
  outcome::result<std::vector<T>> decodeCollection(Stream &stream) {
    // determine number of items
    OUTCOME_TRY(collection_size, compact::decodeInteger(stream));

    auto item_count = collection_size.convert_to<uint64_t>();

    BigInteger required_bytes = collection_size * sizeof(T);
    if (required_bytes > std::numeric_limits<uint64_t>::max()) {
      return DecodeError::kTooManyItems;
    }

    if (!stream.hasMore(required_bytes.convert_to<uint64_t>())) {
      return DecodeError::kNotEnoughData;
    }

    std::vector<T> decoded_collection;
    decoded_collection.reserve(item_count);

    // parse items one by one
    TypeDecoder<T> type_decoder{};
    for (uint64_t i = 0; i < item_count; ++i) {
      OUTCOME_TRY(r, type_decoder.decode(stream));
      decoded_collection.push_back(r);
    }

    return decoded_collection;
  }
}  // namespace kagome::common::scale::collection

#endif  // KAGOME_SCALE_ADVANCED_HPP
