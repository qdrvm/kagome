/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UNUSED
#define KAGOME_UNUSED

#include "common/empty.hpp"
#include "common/tagged.hpp"

namespace kagome {

  enum UnusedError {
    AttemptToEncodeUnused = 1,
    AttemptToDecodeUnused,
  };

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

  /// To raise failure while attempt to encode unused entity
  template <size_t N>
  inline ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                                 const Unused<N> &) {
    ::scale::raise(UnusedError::AttemptToEncodeUnused);
    return s;
  }

  /// To raise failure while attempt to decode unused entity
  template <size_t N>
  inline ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                                 const Unused<N> &) {
    ::scale::raise(UnusedError::AttemptToDecodeUnused);
    return s;
  }

}  // namespace kagome

#endif  // KAGOME_UNUSED
