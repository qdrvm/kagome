/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/optional_bool.hpp"
#include "scale/scale_decoder_stream.hpp"
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

    return s << static_cast<uint8_t>(result);
  }

  ScaleDecoderStream &operator>>(ScaleDecoderStream &s,
                                 std::optional<bool> &b) {
    auto byte = s.nextByte();

    switch (byte) {
      case static_cast<uint8_t>(OptionalBool::NONE):
        b = std::nullopt;
        break;
      case static_cast<uint8_t>(OptionalBool::FALSE):
        b = false;
      case static_cast<uint8_t>(OptionalBool::TRUE):
        b = true;
      default:
        common::raise(DecodeError::UNEXPECTED_VALUE);
    }

    return s;
  }

}  // namespace kagome::scale
