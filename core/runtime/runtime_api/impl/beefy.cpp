/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/beefy.hpp"

#include "runtime/common/runtime_execution_error.hpp"
#include "runtime/executor.hpp"

namespace kagome::runtime {
  BeefyApiImpl::BeefyApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<std::optional<primitives::BlockNumber>> BeefyApiImpl::genesis(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    auto r = executor_->call<std::optional<primitives::BlockNumber>>(
        ctx, "BeefyApi_beefy_genesis");
    if (r) {
      return std::move(r.value());
    }
    if (r.error() == RuntimeExecutionError::EXPORT_FUNCTION_NOT_FOUND) {
      return std::nullopt;
    }
    return r.error();
  }

  outcome::result<std::optional<consensus::beefy::ValidatorSet>>
  BeefyApiImpl::validatorSet(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<consensus::beefy::ValidatorSet>>(
        ctx, "BeefyApi_validator_set");
  }

  outcome::result<void>
  BeefyApiImpl::submit_report_double_voting_unsigned_extrinsic(
      const primitives::BlockHash &block,
      const consensus::beefy::DoubleVotingProof &equivocation_proof,
      const primitives::OpaqueKeyOwnershipProof &key_owner_proof) const {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<void>(
        ctx,
        "BeefyApi_submit_report_double_voting_unsigned_extrinsic",
        equivocation_proof,
        key_owner_proof);
  }

  outcome::result<std::optional<primitives::OpaqueKeyOwnershipProof>>
  BeefyApiImpl::generate_key_ownership_proof(
      const primitives::BlockHash &block,
      consensus::beefy::AuthoritySetId set_id,
      const crypto::EcdsaPublicKey &authority_id) const {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<primitives::OpaqueKeyOwnershipProof>>(
        ctx, "BeefyApi_generate_key_ownership_proof", set_id, authority_id);
  }
}  // namespace kagome::runtime
