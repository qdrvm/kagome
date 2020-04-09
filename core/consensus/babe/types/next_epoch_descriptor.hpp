/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_TYPES_NEXT_EPOCH_DESCRIPTOR_HPP
#define KAGOME_CORE_CONSENSUS_BABE_TYPES_NEXT_EPOCH_DESCRIPTOR_HPP

#include "primitives/authority.hpp"

namespace kagome::consensus {

  /// Information about the epoch after next epoch
  struct NextEpochDescriptor {
    /// The authorities.
    std::vector<primitives::Authority> authorities;

    /// The value of randomness to use for the slot-assignment.
    Randomness randomness;

    bool operator==(const NextEpochDescriptor &rhs) const {
      return authorities == rhs.authorities and randomness == rhs.randomness;
    }
    bool operator!=(const NextEpochDescriptor &rhs) const {
      return not operator==(rhs);
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const NextEpochDescriptor &ned) {
    return s << ned.authorities << ned.randomness;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, NextEpochDescriptor &ned) {
    return s >> ned.authorities >> ned.randomness;
  }

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_TYPES_NEXT_EPOCH_DESCRIPTOR_HPP
