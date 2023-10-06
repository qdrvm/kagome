/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_PROVISIONER_TYPES_HPP
#define KAGOME_DISPUTE_PROVISIONER_TYPES_HPP

#include "dispute_coordinator/types.hpp"
#include "network/types/collator_messages.hpp"

namespace kagome::dispute {

  using network::BitfieldDistribution;

  /// A per-relay-parent state for the provisioning subsystem.
  struct PerRelayParent {
    ActivatedLeaf leaf;
    std::vector<CandidateReceipt> backed_candidates;
    // std::vector<SignedAvailabilityBitfield> signed_bitfields;
    bool is_inherent_ready = false;
    // std::vector<oneshot::Sender<ProvisionerInherentData>> awaiting_inherent;
    // PerLeafSpan span;
  };

  enum Misbehavior {};  // FIXME
  using BackedCandidate = Tagged<CandidateReceipt, struct BackedCandidateTag>;
  using MisbehaviorReport =
      std::tuple<primitives::BlockHash, ValidatorIndex, Misbehavior>;
  using Dispute = std::tuple<primitives::BlockHash, ValidatorSignature>;

  /// This data becomes intrinsics or extrinsics which should be included in a
  /// future relay chain block.
  // It needs to be cloneable because multiple potential block authors can
  // request copies.
  using ProvisionableData = boost::variant<
      /// This bitfield indicates the availability of various candidate blocks.
      BitfieldDistribution,

      /// The Candidate Backing subsystem believes that this candidate is valid,
      /// pending availability.
      CandidateReceipt,  // BackedCandidate

      /// Misbehavior reports are self-contained proofs of validator
      /// misbehavior.
      MisbehaviorReport,

      /// Disputes trigger a broad dispute resolution process.
      Dispute>;

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_PARTICIPATION_TYPES_HPP
