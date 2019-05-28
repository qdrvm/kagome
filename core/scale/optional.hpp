/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OPTIONAL_HPP
#define KAGOME_OPTIONAL_HPP

#include "common/buffer.hpp"
#include "scale/scale_error.hpp"
#include "scale/type_decoder.hpp"
#include "scale/types.hpp"

namespace kagome::scale::optional {
  /**
   * @brief decodeOptional function decodes optional value from stream
   * @tparam T optional value content type
   * @param stream source stream
   * @return decoded optional value or error
   */
  template <class T>
  outcome::result<std::optional<T>> decodeOptional(common::ByteStream &stream) {
    auto flag = stream.nextByte();
    if (!flag.has_value()) {
      return outcome::failure(DecodeError::NOT_ENOUGH_DATA);
    }

    if (*flag != 1) {
      return std::nullopt;
    }

    TypeDecoder<T> codec{};
    OUTCOME_TRY(result, codec.decode(stream));

    return std::optional<T>{result};
  }
}  // namespace kagome::scale::optional

#endif  // KAGOME_OPTIONAL_HPP
