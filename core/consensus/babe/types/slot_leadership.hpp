/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "consensus/babe/types/babe_configuration.hpp"
#include "consensus/babe/types/slot_type.hpp"

namespace kagome::consensus::babe {

  class SlotLeadership {
   public:
    SlotType slot_type;
    AuthorityIndex authority_index;
    std::shared_ptr<crypto::Sr25519Keypair> keypair;
    crypto::VRFOutput vrf_output;
  };

}  // namespace kagome::consensus::babe
