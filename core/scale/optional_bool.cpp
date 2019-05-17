/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/optional_bool.hpp"

namespace kagome::scale::optional {
  template <>
  outcome::result<void> encodeOptional<bool>(
      const std::optional<bool> &optional, common::Buffer &out) {
    uint8_t result = 2;  // true

    if (!optional.has_value()) {  // none
      result = 0;
    } else if (!*optional) {  // false
      result = 1;
    }

    out.putUint8(result);
    return outcome::success();
  }

  template <>
  outcome::result<std::optional<bool>> decodeOptional(
      common::ByteStream &stream) {
    auto byte = stream.nextByte();
    if (!byte.has_value()) {
      return outcome::failure(DecodeError::NOT_ENOUGH_DATA);
    }

    switch (*byte) {
      case 0:
        return outcome::success(std::nullopt);
      case 1:
        return std::optional<bool>{false};
      case 2:
        return std::optional<bool>{true};
      default:
        break;
    }

    return outcome::failure(DecodeError::UNEXPECTED_VALUE);
  }
}  // namespace kagome::scale::optional
