/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/grandpa_api.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  GrandpaApiImpl::GrandpaApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<GrandpaApi::Authorities> GrandpaApiImpl::authorities(
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
    return executor_->call<Authorities>(ctx, "GrandpaApi_grandpa_authorities");
  }

  outcome::result<GrandpaApi::AuthoritySetId> GrandpaApiImpl::current_set_id(
      const primitives::BlockHash &block_hash) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
    return executor_->call<AuthoritySetId>(ctx, "GrandpaApi_current_set_id");
  }

  outcome::result<std::optional<consensus::grandpa::OpaqueKeyOwnershipProof>>
  GrandpaApiImpl::generate_key_ownership_proof(
      const primitives::BlockHash &block_hash,
      consensus::SlotNumber slot,
      consensus::grandpa::AuthorityId authority_id) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
    return executor_
        ->call<std::optional<consensus::grandpa::OpaqueKeyOwnershipProof>>(
            ctx, "GrandpaApi_generate_key_ownership_proof", slot, authority_id);
  }

  outcome::result<void>
  GrandpaApiImpl::submit_report_equivocation_unsigned_extrinsic(
      const primitives::BlockHash &block_hash,
      consensus::grandpa::EquivocationProof equivocation_proof,
      consensus::grandpa::OpaqueKeyOwnershipProof key_owner_proof) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
    return executor_->call<void>(
        ctx,
        "GrandpaApi_submit_report_equivocation_unsigned_extrinsic",
        equivocation_proof,
        key_owner_proof);
  }

}  // namespace kagome::runtime
