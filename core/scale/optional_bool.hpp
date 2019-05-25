/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_OPTIONAL_BOOL_HPP
#define KAGOME_CORE_SCALE_OPTIONAL_BOOL_HPP

#include "scale/optional.hpp"

/**
 * Optional bool is a special case of optional values in SCALE
 * Optional bools are encoded using only 1 byte
 * 0 means no value, 1 means false, 2 means true
 */

namespace kagome::scale {
  class ScaleEncoderStream;

  /**
   * @brief scale-encodes optional bool value
   * @param s reference to scale-encoder stream
   * @param b optional bool value to encode
   * @return reference to stream
   */
  ScaleEncoderStream &operator<<(ScaleEncoderStream &s, const std::optional<bool> &b);
}

namespace kagome::scale::optional {
  /**
   * @brief specialization of decodeOptional function for optional bool
   * @param optional bool value
   * @return decoded value or error
   */
  template <>
  outcome::result<std::optional<bool>> decodeOptional(
      common::ByteStream &stream);

}  // namespace kagome::scale::optional

#endif  // KAGOME_CORE_SCALE_OPTIONAL_BOOL_HPP
