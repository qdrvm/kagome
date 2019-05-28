/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_ADVANCED_HPP
#define KAGOME_SCALE_ADVANCED_HPP

#include "common/buffer.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/scale_encoder_stream.hpp"
#include "scale/type_decoder.hpp"

namespace kagome::scale::collection {
  /**
   * @brief decodeCollection function decodes collection containing items of
   * same specified type
   * @tparam T collection item type
   * @param stream source stream
   * @param decode a functor that decodes an object of type T
   * @return decoded collection or error
   */
  template <class T>
  outcome::result<std::vector<T>> decodeCollection(
      common::ByteStream &stream,
      std::function<outcome::result<T>(common::ByteStream &)> decode_f) {
    // determine number of items
    OUTCOME_TRY(collection_size, compact::decodeInteger(stream));

    auto item_count = collection_size.convert_to<uint64_t>();

    BigInteger required_bytes = collection_size * sizeof(T);
    if (required_bytes > std::numeric_limits<uint64_t>::max()) {
      return DecodeError::TOO_MANY_ITEMS;
    }

    std::vector<T> decoded_collection;
    decoded_collection.reserve(item_count);
    // TODO (yuraz): PRE-119 refactor
    // parse items one by one
    for (uint64_t i = 0; i < item_count; ++i) {
      OUTCOME_TRY(r, decode_f(stream));
      decoded_collection.push_back(r);
    }

    return decoded_collection;
  }

  template <class T>
  outcome::result<std::vector<T>> decodeCollection(common::ByteStream &stream) {
    TypeDecoder<T> type_decoder{};
    using std::placeholders::_1;
    return decodeCollection<T>(
        stream, std::bind(&TypeDecoder<T>::decode, &type_decoder, _1));
  }

  /**
   * @brief decodes string from stream
   * @param stream source of encoded bytes
   * @return decoded string or error
   */
  outcome::result<std::string> decodeString(common::ByteStream &stream);

}  // namespace kagome::scale::collection

#endif  // KAGOME_SCALE_ADVANCED_HPP
