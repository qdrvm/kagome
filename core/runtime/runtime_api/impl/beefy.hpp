/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/beefy.hpp"

namespace kagome::runtime {
  class Executor;

  class BeefyApiImpl : public BeefyApi {
   public:
    BeefyApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<std::optional<primitives::BlockNumber>> genesis(
        const primitives::BlockHash &block) override;
    outcome::result<std::optional<consensus::beefy::ValidatorSet>> validatorSet(
        const primitives::BlockHash &block) override;
    outcome::result<void> submit_report_double_voting_unsigned_extrinsic(
        const primitives::BlockHash &block,
        const consensus::beefy::DoubleVotingProof &equivocation_proof,
        const primitives::OpaqueKeyOwnershipProof &key_owner_proof)
        const override;
    outcome::result<std::optional<primitives::OpaqueKeyOwnershipProof>>
    generate_key_ownership_proof(
        const primitives::BlockHash &block,
        consensus::beefy::AuthoritySetId set_id,
        const crypto::EcdsaPublicKey &authority_id) const override;

   private:
    std::shared_ptr<Executor> executor_;
  };
}  // namespace kagome::runtime
