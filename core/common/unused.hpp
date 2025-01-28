/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/enum_error_code.hpp>
#include <qtils/unused.hpp>
#include <scale/scale.hpp>

namespace kagome {

  enum class UnusedError : uint8_t {
    AttemptToEncodeUnused = 1,
    AttemptToDecodeUnused,
  };
  Q_ENUM_ERROR_CODE(UnusedError) {
    using E = decltype(e);
    switch (e) {
      case E::AttemptToEncodeUnused:
        return "Attempt to encode a value that must be unused";
      case E::AttemptToDecodeUnused:
        return "Attempt to decode a value that must be unused";
    }
    abort();
  }

  /// Special zero-size-type for some things
  ///  (e.g., dummy types of variant, unsupported or experimental).
  template <size_t N>
  using Unused = qtils::Unused<N>;

  /// To raise failure while attempt to encode unused entity
  template <size_t N>
  [[noreturn]] ::scale::ScaleEncoderStream &operator<<(
      ::scale::ScaleEncoderStream &, const Unused<N> &) {
    ::scale::raise(UnusedError::AttemptToEncodeUnused);
  }

  /// To raise failure while attempt to decode unused entity
  template <size_t N>
  [[noreturn]] ::scale::ScaleDecoderStream &operator>>(
      ::scale::ScaleDecoderStream &, Unused<N> &) {
    ::scale::raise(UnusedError::AttemptToDecodeUnused);
  }

}  // namespace kagome
