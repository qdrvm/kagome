/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "consensus/sassafras/types/authority.hpp"
#include "consensus/sassafras/types/ticket.hpp"

namespace kagome::consensus::sassafras {

  class SlotLeadership {
   public:
    AuthorityIndex authority_index;
    std::shared_ptr<crypto::BandersnatchKeypair> keypair;
    crypto::bandersnatch::vrf::VrfSignature signature;
    std::optional<TicketClaim> ticket_claim;
  };

}  // namespace kagome::consensus::sassafras
