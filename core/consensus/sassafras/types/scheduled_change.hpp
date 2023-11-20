/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/unused.hpp"
#include "consensus/sassafras/types/authority.hpp"
#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::sassafras {

  /// Information about the next epoch.
  ///
  /// This is mandatory in the first block of each epoch.
  struct NextEpochDescriptor {
    SCALE_TIE(3);

    /// Authorities list.
    Authorities authorities;

    /// Epoch randomness.
    Randomness randomness;

    /// Epoch configurable parameters.
    ///
    /// If not present previous epoch parameters are used.
    std::optional<EpochConfiguration> config;
  };

  struct OnDisabled {
    SCALE_TIE(1);
    AuthorityIndex authority_index = 0;
  };

}  // namespace kagome::consensus::sassafras
