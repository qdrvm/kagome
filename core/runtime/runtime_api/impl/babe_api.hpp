/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/babe_api.hpp"

namespace kagome::runtime {

  class Executor;

  class BabeApiImpl final : public BabeApi {
   public:
    explicit BabeApiImpl(std::shared_ptr<Executor> executor);

    outcome::result<consensus::babe::BabeConfiguration> configuration(
        const primitives::BlockHash &block) override;

    outcome::result<consensus::babe::Epoch> next_epoch(
        const primitives::BlockHash &block) override;

    outcome::result<std::optional<consensus::babe::OpaqueKeyOwnershipProof>>
    generate_key_ownership_proof(
        const primitives::BlockHash &block_hash,
        consensus::SlotNumber slot,
        consensus::babe::AuthorityId authority_id) override;

    outcome::result<void> submit_report_equivocation_unsigned_extrinsic(
        const primitives::BlockHash &block_hash,
        consensus::babe::EquivocationProof equivocation_proof,
        consensus::babe::OpaqueKeyOwnershipProof key_owner_proof) override;

   private:
    std::shared_ptr<Executor> executor_;
  };

}  // namespace kagome::runtime
