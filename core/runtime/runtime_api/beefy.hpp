/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"
#include "primitives/opaque_key_ownership_proof.hpp"

namespace kagome::runtime {
  class BeefyApi {
   public:
    virtual ~BeefyApi() = default;

    /**
     * Get genesis if beefy is supported.
     */
    virtual outcome::result<std::optional<primitives::BlockNumber>> genesis(
        const primitives::BlockHash &block) = 0;

    /**
     * Get validator set.
     */
    virtual outcome::result<std::optional<consensus::beefy::ValidatorSet>>
    validatorSet(const primitives::BlockHash &block) = 0;

    /**
     * Report equivocation.
     */
    virtual outcome::result<void>
    submit_report_double_voting_unsigned_extrinsic(
        const primitives::BlockHash &block,
        const consensus::beefy::DoubleVotingProof &equivocation_proof,
        const primitives::OpaqueKeyOwnershipProof &key_owner_proof) const = 0;

    /**
     * Generate key ownership proof.
     */
    virtual outcome::result<std::optional<primitives::OpaqueKeyOwnershipProof>>
    generate_key_ownership_proof(
        const primitives::BlockHash &block,
        consensus::beefy::AuthoritySetId set_id,
        const crypto::EcdsaPublicKey &authority_id) const = 0;
  };
}  // namespace kagome::runtime
