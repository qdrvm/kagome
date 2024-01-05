/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/types/authority.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "scale/tie.hpp"

namespace kagome::consensus::sassafras {

  /// Epoch slot claim digest entry.
  ///
  /// This is mandatory for each block.
  struct SlotClaim {
    //    SCALE_TIE(4);

    /// Authority index that claimed the slot.
    AuthorityIndex authority_index;
    /// Corresponding slot number.
    SlotNumber slot_number;
    /// Slot claim VRF signature.
    crypto::bandersnatch::vrf::VrfSignature signature;
    /// Ticket auxiliary information for claim check.
    std::optional<TicketClaim> ticket_claim;

    friend scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                                 const SlotClaim &x) {
      s << x.authority_index;
      s << x.slot_number;
      s << x.signature;
      s << x.ticket_claim;
      return s;
    }

    friend scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                                 SlotClaim &x) {
      s >> x.authority_index;
      s >> x.slot_number;
      s >> x.signature;
      s >> x.ticket_claim;
      return s;
    }
  };

}  // namespace kagome::consensus::sassafras
