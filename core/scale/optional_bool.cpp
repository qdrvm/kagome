/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/optional_bool.hpp"

namespace kagome::scale::optional {
  /// @brief OptionalBool is internal extended bool type
  enum class OptionalBool : uint8_t { NONE = 0u, FALSE = 1u, TRUE = 2u };

  template <>
  outcome::result<void> encodeOptional<bool>(
      const std::optional<bool> &optional, common::Buffer &out) {
    auto result = OptionalBool::TRUE;

    if (!optional.has_value()) {
      result = OptionalBool::NONE;
    } else if (!*optional) {
      result = OptionalBool::FALSE;
    }

    out.putUint8(static_cast<uint8_t>(result));
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
      case static_cast<uint8_t>(OptionalBool::NONE):
        return outcome::success(std::nullopt);
      case static_cast<uint8_t>(OptionalBool::FALSE):
        return std::optional<bool>{false};
      case static_cast<uint8_t>(OptionalBool::TRUE):
        return std::optional<bool>{true};
      default:
        break;
    }

    return outcome::failure(DecodeError::UNEXPECTED_VALUE);
  }
}  // namespace kagome::scale::optional
