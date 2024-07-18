/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

#include "consensus/sassafras/bandersnatch.hpp"
#include "consensus/sassafras/types/equivocation_proof.hpp"
#include "consensus/sassafras/types/opaque_key_ownership_proof.hpp"
#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "consensus/timeline/types.hpp"
#include "crypto/bandersnatch/vrf.hpp"

namespace kagome::runtime {

  /// API necessary for block authorship with Sassafras.
  class SassafrasApi {
   public:
    virtual ~SassafrasApi() = default;

    /// Get ring context to be used for ticket construction and verification.
    virtual outcome::result<
        std::optional<crypto::bandersnatch::vrf::RingContext>>
    ring_context(const primitives::BlockHash &block) = 0;

    /// Submit next epoch validator tickets via an unsigned extrinsic.
    /// This method returns `false` when creation of the extrinsics fails.
    virtual outcome::result<bool> submit_tickets_unsigned_extrinsic(
        const primitives::BlockHash &block,
        const std::vector<consensus::sassafras::TicketEnvelope> &tickets) = 0;

    /// Get ticket id associated to the given slot.
    virtual outcome::result<std::optional<consensus::sassafras::TicketId>>
    slot_ticket_id(const primitives::BlockHash &block,
                   consensus::SlotNumber slot) = 0;

    /// Get ticket id and data associated to the given slot.
    virtual outcome::result<
        std::optional<std::tuple<consensus::sassafras::TicketId,
                                 consensus::sassafras::TicketBody>>>
    slot_ticket(const primitives::BlockHash &block,
                consensus::SlotNumber slot) = 0;

    /// Current epoch information.
    virtual outcome::result<consensus::sassafras::Epoch> current_epoch(
        const primitives::BlockHash &block) = 0;

    /// Next epoch information.
    virtual outcome::result<consensus::sassafras::Epoch> next_epoch(
        const primitives::BlockHash &block) = 0;

    /// Generates a proof of key ownership for the given authority in the
    /// current epoch.
    ///
    /// An example usage of this module is coupled with the session historical
    /// module to prove that a given authority key is tied to a given staking
    /// identity during a specific session.
    ///
    /// Proofs of key ownership are necessary for submitting equivocation
    /// reports.
    virtual outcome::result<
        std::optional<consensus::sassafras::OpaqueKeyOwnershipProof>>
    generate_key_ownership_proof(
        const primitives::BlockHash &block,
        const consensus::sassafras::AuthorityId &authority_id) = 0;

    /// Submits an unsigned extrinsic to report an equivocation.
    ///
    /// The caller must provide the equivocation proof and a key ownership proof
    /// (should be obtained using `generate_key_ownership_proof`). The extrinsic
    /// will be unsigned and should only be accepted for local authorship (not
    /// to be broadcast to the network). This method returns `false` when
    /// creation of the extrinsic fails.
    ///
    /// Only useful in an offchain context.
    virtual outcome::result<void> submit_report_equivocation_unsigned_extrinsic(
        const primitives::BlockHash &block,
        const consensus::sassafras::EquivocationProof &equivocation_proof,
        const consensus::sassafras::OpaqueKeyOwnershipProof
            &key_owner_proof) = 0;

    /// Returns a list of all disabled validators at the given block.
    virtual outcome::result<std::vector<consensus::AuthorityIndex>>
    disabled_validators(const primitives::BlockHash &block) = 0;
  };

}  // namespace kagome::runtime
