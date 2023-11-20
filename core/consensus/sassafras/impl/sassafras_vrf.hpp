/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "consensus/timeline/types.hpp"

namespace kagome::consensus::sassafras::vrf {

  /// VRF input to generate the ticket id.
  VrfInput ticket_id_input(const Randomness &randomness,
                           AttemptsNumber attempt,
                           EpochNumber epoch);

}  // namespace kagome::consensus::sassafras::vrf
