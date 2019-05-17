/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_OPTIONAL_BOOL_HPP
#define KAGOME_CORE_SCALE_OPTIONAL_BOOL_HPP

#include "scale/optional.hpp"

namespace kagome::scale::optional {
  /**
   * @brief specialization of encodeOptional function for optional bool
   * @param optional bool value
   * @return encoded value
   */
  template <>
  outcome::result<void> encodeOptional<bool>(
      const std::optional<bool> &optional, common::Buffer &out);

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
