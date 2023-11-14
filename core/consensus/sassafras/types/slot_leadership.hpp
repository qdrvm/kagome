/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "consensus/sassafras/types/ticket.hpp"

namespace kagome::consensus::sassafras {

  class SlotLeadership {
   public:
    primitives::AuthorityIndex authority_index;
    std::shared_ptr<crypto::BandersnatchKeypair> keypair;
    std::optional<EphemeralSeed> erased_seed;
  };

}  // namespace kagome::consensus::sassafras
