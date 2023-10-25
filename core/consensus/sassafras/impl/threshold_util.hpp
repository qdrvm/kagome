/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/types/ticket.hpp"
#include "consensus/timeline/types.hpp"
#include "primitives/authority.hpp"

namespace kagome::consensus::sassafras {

  /// Computes ticket-id maximum allowed value for a given epoch.
  ///
  /// Only ticket identifiers below this threshold should be considered for slot
  /// assignment.
  ///
  /// The value is computed as
  /// `TicketId::MAX*(redundancy*slots)/(attempts*validators)`
  ///
  /// Where:
  /// - `redundancy`: redundancy factor;
  /// - `slots`: number of slots in epoch;
  /// - `attempts`: max number of tickets attempts per validator;
  /// - `validators`: number of validators in epoch.
  ///
  /// If `attempts * validators = 0` then we return 0.
  TicketId ticket_id_threshold(RedundancyFactor redundancy,
                               SlotNumber slots,
                               AttemptsNumber attempts,
                               uint32_t validators);

  /// Calculates the primary selection threshold for a given authority, taking
  /// into account `c` (`1 - c` represents the probability of a slot being
  /// empty).
  /// https://github.com/paritytech/substrate/blob/7010ec7716e0edf97d61a29bd0c337648b3a57ae/core/consensus/babe/src/authorship.rs#L30
  Threshold calculateThreshold(const std::pair<uint64_t, uint64_t> &ratio,
                               const primitives::AuthorityList &authorities,
                               primitives::AuthorityIndex authority_index);

}  // namespace kagome::consensus::sassafras
