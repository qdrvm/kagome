/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <outcome/outcome.hpp>

#include "common/tagged.hpp"
#include "consensus/babe/types/babe_configuration.hpp"
#include "consensus/babe/types/equivocation_proof.hpp"
#include "primitives/common.hpp"

namespace kagome::runtime {

  /**
   * Api to invoke runtime entries related for Babe algorithm
   */
  class BabeApi {
   public:
    virtual ~BabeApi() = default;

    /**
     * Get configuration for the babe
     * @return Babe configuration
     */
    virtual outcome::result<consensus::babe::BabeConfiguration> configuration(
        const primitives::BlockHash &block) = 0;

    /**
     * Get next config from last digest.
     */
    virtual outcome::result<consensus::babe::Epoch> next_epoch(
        const primitives::BlockHash &block) = 0;

    /// Generates a proof of key ownership for the given authority in the
    /// current epoch. An example usage of this module is coupled with the
    /// session historical module to prove that a given authority key is
    /// tied to a given staking identity during a specific session. Proofs
    /// of key ownership are necessary for submitting equivocation reports.
    /// NOTE: even though the API takes a `slot` as parameter the current
    /// implementations ignores this parameter and instead relies on this
    /// method being called at the correct block height, i.e. any point at
    /// which the epoch for the given slot is live on-chain. Future
    /// implementations will instead use indexed data through an offchain
    /// worker, not requiring older states to be available.
    virtual outcome::result<
        std::optional<consensus::babe::OpaqueKeyOwnershipProof>>
    generate_key_ownership_proof(const primitives::BlockHash &block_hash,
                                 consensus::SlotNumber slot,
                                 consensus::babe::AuthorityId authority_id) = 0;

    /// Submits an unsigned extrinsic to report an equivocation. The caller
    /// must provide the equivocation proof and a key ownership proof
    /// (should be obtained using `generate_key_ownership_proof`). The
    /// extrinsic will be unsigned and should only be accepted for local
    /// authorship (not to be broadcast to the network). This method returns
    /// `None` when creation of the extrinsic fails, e.g. if equivocation
    /// reporting is disabled for the given runtime (i.e. this method is
    /// hardcoded to return `None`). Only useful in an offchain context.
    virtual outcome::result<void> submit_report_equivocation_unsigned_extrinsic(
        const primitives::BlockHash &block_hash,
        consensus::babe::EquivocationProof equivocation_proof,
        consensus::babe::OpaqueKeyOwnershipProof key_owner_proof) = 0;

    /// Returns a list of all disabled validators at the given block.
    virtual outcome::result<std::vector<consensus::AuthorityIndex>>
    disabled_validators(const primitives::BlockHash &block) = 0;
  };

}  // namespace kagome::runtime
