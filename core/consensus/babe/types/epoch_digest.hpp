/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_EPOCHDIGEST
#define KAGOME_CONSENSUS_EPOCHDIGEST

#include "consensus/babe/common.hpp"
#include "primitives/authority.hpp"

namespace kagome::consensus {

  /// Data are corresponding to the epoch
  struct EpochDigest {
    /// The authorities actual for corresponding epoch
    primitives::AuthorityList authorities;

    /// The value of randomness to use for the slot-assignment.
    Randomness randomness;

    bool operator==(const EpochDigest &rhs) const {
      return authorities == rhs.authorities and randomness == rhs.randomness;
    }
    bool operator!=(const EpochDigest &rhs) const {
      return not operator==(rhs);
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const EpochDigest &digest) {
    return s << digest.authorities << digest.randomness;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, EpochDigest &digest) {
    return s >> digest.authorities >> digest.randomness;
  }

}  // namespace kagome::consensus

#endif  // KAGOME_CONSENSUS_EPOCHDIGEST
