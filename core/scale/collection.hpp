/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_ADVANCED_HPP
#define KAGOME_SCALE_ADVANCED_HPP

#include "common/buffer.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/type_decoder.hpp"
#include "scale/type_encoder.hpp"

namespace kagome::scale::collection {
  /**
   *  @brief encodeCollection function scale-encodes
   *         collection of same type values
   *  @tparam T Item type stored in collection
   *  @param collection source vector items
   *  @return true if operation succeeded and false otherwise
   */
  template <class T>
  outcome::result<void> encodeCollection(const gsl::span<T> collection,
                                         common::Buffer &out) {
    common::Buffer encoded_collection;
    OUTCOME_TRY(compact::encodeInteger(collection.size(), encoded_collection));

    TypeEncoder<T> encoder{};
    for (auto i = 0; i < collection.size(); ++i) {
      OUTCOME_TRY(encoder.encode(collection[i], encoded_collection));
    }

    out.putBuffer(encoded_collection);

    return outcome::success();
  }

  template <class T>
  outcome::result<void> encodeCollection(const std::vector<T> collection,
                                         common::Buffer &out) {
    common::Buffer encoded_collection;
    OUTCOME_TRY(compact::encodeInteger(collection.size(), encoded_collection));

    TypeEncoder<T> encoder{};
    for (auto &item : collection) {
      OUTCOME_TRY(encoder.encode(item, encoded_collection));
    }

    out.putBuffer(encoded_collection);

    return outcome::success();
  }

  /**
   * @brief encodes buffer as collection of bytes
   * @param buf bytes to encode
   * @param out output buffer
   * @return true if operation succeeded false otherwise
   */
  outcome::result<void> encodeBuffer(const common::Buffer &buf,
                                     common::Buffer &out);

  /**
   * @brief encodes std::string as collection of bytes
   * @param string source value
   * @param out output buffer
   * @return true if operation succeedes false otherwise
   */
  outcome::result<void> encodeString(std::string_view string,
                                     common::Buffer &out);

  /**
   * @brief decodeCollection function decodes collection containing items of
   * same specified type
   * @tparam T collection item type
   * @param stream source stream
   * @return decoded collection or error
   */
  template <class T>
  outcome::result<std::vector<T>> decodeCollection(common::ByteStream &stream) {
    // determine number of items
    OUTCOME_TRY(collection_size, compact::decodeInteger(stream));

    auto item_count = collection_size.convert_to<uint64_t>();

    BigInteger required_bytes = collection_size * sizeof(T);
    if (required_bytes > std::numeric_limits<uint64_t>::max()) {
      return DecodeError::TOO_MANY_ITEMS;
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

  /**
   * @brief decodes string from stream
   * @param stream source of encoded bytes
   * @return decoded string or error
   */
  outcome::result<std::string> decodeString(common::ByteStream &stream);
}  // namespace kagome::scale::collection
#endif  // KAGOME_SCALE_ADVANCED_HPP
