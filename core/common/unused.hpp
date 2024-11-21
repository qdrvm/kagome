/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

#include <qtils/enum_error_code.hpp>

#include "common/empty.hpp"
#include "common/tagged.hpp"

namespace kagome {

  enum UnusedError : uint8_t {
    AttemptToEncodeUnused = 1,
    AttemptToDecodeUnused,
  };
  Q_ENUM_ERROR_CODE(UnusedError) {
    using E = decltype(e);
    switch (e) {
      case E::AttemptToEncodeUnused:
        return "AttemptToEncodeUnused";
      case E::AttemptToDecodeUnused:
        return "AttemptToDecodeUnused";
    }
    abort();
  }

  /// Number-based marker-type for using as tag
  template <size_t Num>
  struct NumTag {
   private:
    static constexpr size_t tag = Num;
  };

  /// Special zero-size-type for some things
  ///  (e.g., dummy types of variant, unsupported or experimental).
  template <size_t N>
  using Unused = Tagged<Empty, NumTag<N>>;

  template <size_t N>
  constexpr bool operator==(const Unused<N> &, const Unused<N> &) {
    return true;
  }


  /// To raise failure while attempt to decode unused entity
  template <size_t N>
  inline ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                                 const Unused<N> &) {
    ::scale::raise(UnusedError::AttemptToDecodeUnused);
    return s;
  }

}  // namespace kagome
