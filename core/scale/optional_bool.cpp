/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/optional_bool.hpp"
#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {
  /// @brief OptionalBool is internal extended bool type
  enum class OptionalBool : uint8_t { NONE = 0u, FALSE = 1u, TRUE = 2u };

  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const std::optional<bool> &b) {
    auto result = OptionalBool::TRUE;

    if (!b.has_value()) {
      result = OptionalBool::NONE;
    } else if (!*b) {
      result = OptionalBool::FALSE;
    }

    return s <<static_cast<uint8_t>(result);
  }
}  // namespace kagome::scale

namespace kagome::scale::optional {
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
