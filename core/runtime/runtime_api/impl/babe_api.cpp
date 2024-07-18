/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/babe_api.hpp"

#include "runtime/common/runtime_execution_error.hpp"
#include "runtime/executor.hpp"

namespace kagome::runtime {

  BabeApiImpl::BabeApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<consensus::babe::BabeConfiguration>
  BabeApiImpl::configuration(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<consensus::babe::BabeConfiguration>(
        ctx, "BabeApi_configuration");
  }

  outcome::result<consensus::babe::Epoch> BabeApiImpl::next_epoch(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<consensus::babe::Epoch>(ctx, "BabeApi_next_epoch");
  }

  outcome::result<std::optional<consensus::babe::OpaqueKeyOwnershipProof>>
  BabeApiImpl::generate_key_ownership_proof(
      const primitives::BlockHash &block_hash,
      consensus::SlotNumber slot,
      consensus::babe::AuthorityId authority_id) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
    return executor_
        ->call<std::optional<consensus::babe::OpaqueKeyOwnershipProof>>(
            ctx, "BabeApi_generate_key_ownership_proof", slot, authority_id);
  }

  outcome::result<void>
  BabeApiImpl::submit_report_equivocation_unsigned_extrinsic(
      const primitives::BlockHash &block_hash,
      consensus::babe::EquivocationProof equivocation_proof,
      consensus::babe::OpaqueKeyOwnershipProof key_owner_proof) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block_hash));
    return executor_->call<void>(
        ctx,
        "BabeApi_submit_report_equivocation_unsigned_extrinsic",
        equivocation_proof,
        key_owner_proof);
  }

  outcome::result<std::vector<consensus::AuthorityIndex>>
  BabeApiImpl::disabled_validators(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    auto res = executor_->call<std::vector<consensus::AuthorityIndex>>(
        ctx, "BabeApi_disabled_validators");
    if (res.has_error()
        and res.error() == RuntimeExecutionError::EXPORT_FUNCTION_NOT_FOUND) {
      return std::vector<consensus::AuthorityIndex>{};
    }
    return res;
  }
}  // namespace kagome::runtime
