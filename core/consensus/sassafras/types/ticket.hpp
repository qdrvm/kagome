/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/multiprecision/cpp_int.hpp>

#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "consensus/sassafras/vrf.hpp"
#include "consensus/timeline/types.hpp"
#include "crypto/bandersnatch_types.hpp"
#include "crypto/ed25519_types.hpp"
#include "scale/big_fixed_integers.hpp"
#include "scale/tie.hpp"

// Primitives related to tickets.

namespace kagome::consensus::sassafras {

  using EphemeralSeed = crypto::Ed25519Seed;
  using EphemeralPublic = crypto::Ed25519PublicKey;
  using EphemeralSignature = crypto::Ed25519Signature;

  /// Ticket identifier.
  ///
  /// Its value is the output of a VRF whose inputs cannot be controlled by the
  /// ticket's creator (refer to [`crate::vrf::ticket_id_input`] parameters).
  /// Because of this, it is also used as the ticket score to compare against
  /// the epoch ticket's threshold to decide if the ticket is worth being
  /// considered for slot assignment (refer to [`ticket_id_threshold`]).
  using TicketId = scale::Fixed<scale::uint128_t>;

  /// Ticket data persisted on-chain.
  struct TicketBody {
    SCALE_TIE(3);
    /// Attempt index.
    AttemptsNumber attempt_index;
    /// Ephemeral public key which gets erased when the ticket is claimed.
    EphemeralPublic erased_public;
    /// Ephemeral public key which gets exposed when the ticket is claimed.
    EphemeralPublic revealed_public;
  };

  /// Ticket ring vrf signature.
  using TicketSignature = RingVrfSignature;

  /// Ticket envelope used on during submission.
  struct TicketEnvelope {
    SCALE_TIE(2);
    /// Ticket body.
    TicketBody body;
    /// Ring signature.
    TicketSignature signature;

    friend scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                                 const TicketEnvelope &x) {
      return s  // << x.outputs
                // << x.signature
          ;
    }

    friend scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                                 TicketEnvelope &x) {
      return s  // >> x.outputs
                // >> x.signature
          ;
    }
  };

  /// Ticket claim information filled by the block author.
  struct TicketClaim {
    SCALE_TIE(1);
    /// Signature verified via `TicketBody::erased_public`.
    EphemeralSignature erased_signature;
  };

  struct Ticket {
    SCALE_TIE(3);
    TicketId id;
    TicketEnvelope envelope;
    EphemeralSeed erased_seed;
  };

  using Tickets = std::vector<Ticket>;

}  // namespace kagome::consensus::sassafras
