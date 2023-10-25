/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/types/ticket.hpp"
#include "primitives/authority.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::sassafras {

  /// Epoch slot claim digest entry.
  ///
  /// This is mandatory for each block.
  struct SlotClaim {
    SCALE_TIE(4);

    /// Authority index that claimed the slot.
    primitives::AuthorityIndex authority_index;
    /// Corresponding slot number.
    SlotNumber slot_number;
    /// Slot claim VRF signature.
    VrfSignature signature;
    /// Ticket auxiliary information for claim check.
    std::optional<TicketClaim> ticket_claim;
  };

}  // namespace kagome::consensus::sassafras
