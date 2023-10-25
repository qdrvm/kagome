/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/types.hpp"
#include "primitives/authority.hpp"

namespace kagome::consensus::sassafras {

  using primitives::AuthorityList;

  /// Tickets threshold redundancy factor
  using RedundancyFactor = uint32_t;

  /// Tickets attempts for each validator
  using AttemptsNumber = uint32_t;

  struct EpochConfiguration {
    SCALE_TIE(2);

    /// Tickets threshold redundancy factor
    RedundancyFactor redundancy_factor;
    /// Tickets attempts for each validator
    AttemptsNumber attempts_number;
  };

  /// Sassafras epoch information
  struct Epoch {
    SCALE_TIE(7);

    /// The epoch index.
    EpochNumber epoch_index;
    /// The starting slot of the epoch
    SlotNumber start_slot;
    /// Slot duration in milliseconds
    SlotDuration slot_duration;
    /// Duration of epoch in slots
    EpochLength epoch_length;
    /// Authorities for the epoch
    AuthorityList authorities;
    /// Randomness for the epoch
    Randomness randomness;
    /// Epoch configuration
    EpochConfiguration config;
  };

}  // namespace kagome::consensus::sassafras
