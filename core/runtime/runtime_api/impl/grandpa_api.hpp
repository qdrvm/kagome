/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/grandpa_api.hpp"

namespace kagome::runtime {

  class Executor;

  class GrandpaApiImpl final : public GrandpaApi {
   public:
    GrandpaApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<Authorities> authorities(
        const primitives::BlockHash &block_hash) override;

    outcome::result<AuthoritySetId> current_set_id(
        const primitives::BlockHash &block) override;

    outcome::result<std::optional<consensus::grandpa::OpaqueKeyOwnershipProof>>
    generate_key_ownership_proof(
        const primitives::BlockHash &block_hash,
        consensus::SlotNumber slot,
        consensus::grandpa::AuthorityId authority_id) override;

    outcome::result<void> submit_report_equivocation_unsigned_extrinsic(
        const primitives::BlockHash &block_hash,
        consensus::grandpa::EquivocationProof equivocation_proof,
        consensus::grandpa::OpaqueKeyOwnershipProof key_owner_proof) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime
