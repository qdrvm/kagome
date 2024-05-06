/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/parachain_host.hpp"

#include "common/blob.hpp"
#include "runtime/common/runtime_execution_error.hpp"
#include "runtime/executor.hpp"
#include "runtime/runtime_api/impl/parachain_host_types_serde.hpp"
#include "scale/std_variant.hpp"

namespace kagome::runtime {

  ParachainHostImpl::ParachainHostImpl(
      std::shared_ptr<Executor> executor,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine)
      : executor_{std::move(executor)},
        chain_sub_{std::move(chain_events_engine)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<std::vector<ParachainId>>
  ParachainHostImpl::active_parachains(const primitives::BlockHash &block) {
    OUTCOME_TRY(ref,
                active_parachains_.call(
                    *executor_, block, "ParachainHost_active_parachains"));
    return *ref;
  }

  outcome::result<std::optional<common::Buffer>>
  ParachainHostImpl::parachain_head(const primitives::BlockHash &block,
                                    ParachainId id) {
    OUTCOME_TRY(ref,
                parachain_head_.call(
                    *executor_, block, "ParachainHost_parachain_head", id));
    return *ref;
  }

  outcome::result<std::optional<common::Buffer>>
  ParachainHostImpl::parachain_code(const primitives::BlockHash &block,
                                    ParachainId id) {
    OUTCOME_TRY(ref,
                parachain_code_.call(
                    *executor_, block, "ParachainHost_parachain_code", id));
    return *ref;
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(
        ref, validators_.call(*executor_, block, "ParachainHost_validators"));
    return *ref;
  }

  outcome::result<ValidatorGroupsAndDescriptor>
  ParachainHostImpl::validator_groups(const primitives::BlockHash &block) {
    OUTCOME_TRY(ref,
                validator_groups_.call(
                    *executor_, block, "ParachainHost_validator_groups"));
    return *ref;
  }

  outcome::result<std::vector<CoreState>> ParachainHostImpl::availability_cores(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ref,
                availability_cores_.call(
                    *executor_, block, "ParachainHost_availability_cores"));
    return *ref;
  }

  outcome::result<std::optional<PersistedValidationData>>
  ParachainHostImpl::persisted_validation_data(
      const primitives::BlockHash &block,
      ParachainId id,
      OccupiedCoreAssumption assumption) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<PersistedValidationData>>(
        ctx, "ParachainHost_persisted_validation_data", id, assumption);
  }

  outcome::result<bool> ParachainHostImpl::check_validation_outputs(
      const primitives::BlockHash &block,
      ParachainId id,
      CandidateCommitments outputs) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<bool>(
        ctx, "ParachainHost_check_validation_outputs", id, outputs);
  }

  outcome::result<SessionIndex> ParachainHostImpl::session_index_for_child(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(
        ref,
        session_index_for_child_.call(
            *executor_, block, "ParachainHost_session_index_for_child"));
    return *ref;
  }

  outcome::result<std::optional<ValidationCode>>
  ParachainHostImpl::validation_code(const primitives::BlockHash &block,
                                     ParachainId id,
                                     OccupiedCoreAssumption assumption) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<ValidationCode>>(
        ctx, "ParachainHost_validation_code", id, assumption);
  }

  outcome::result<std::optional<ValidationCode>>
  ParachainHostImpl::validation_code_by_hash(const primitives::BlockHash &block,
                                             ValidationCodeHash hash) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    if constexpr (DISABLE_RUNTIME_LRU) {
      return executor_->call<std::optional<ValidationCode>>(
          ctx, "ParachainHost_validation_code_by_hash", hash);
    }
    if (auto r = validation_code_by_hash_.exclusiveAccess(
            [&](typename decltype(validation_code_by_hash_)::Type
                    &validation_code_by_hash_) {
              auto v = validation_code_by_hash_.get(hash);
              return v ? std::make_optional(v->get()) : std::nullopt;
            })) {
      return *r;
    }
    OUTCOME_TRY(code,
                executor_->call<std::optional<ValidationCode>>(
                    ctx, "ParachainHost_validation_code_by_hash", hash));
    if (code) {
      return validation_code_by_hash_.exclusiveAccess(
          [&](typename decltype(validation_code_by_hash_)::Type
                  &validation_code_by_hash_) {
            return validation_code_by_hash_.put(hash, std::move(*code));
          });
    }
    return std::nullopt;
  }

  outcome::result<std::optional<CommittedCandidateReceipt>>
  ParachainHostImpl::candidate_pending_availability(
      const primitives::BlockHash &block, ParachainId id) {
    OUTCOME_TRY(ref,
                candidate_pending_availability_.call(
                    *executor_,
                    block,
                    "ParachainHost_candidate_pending_availability",
                    id));
    return *ref;
  }

  outcome::result<std::vector<CandidateEvent>>
  ParachainHostImpl::candidate_events(const primitives::BlockHash &block) {
    OUTCOME_TRY(ref,
                candidate_events_.call(
                    *executor_, block, "ParachainHost_candidate_events"));
    return *ref;
  }

  outcome::result<std::optional<SessionInfo>> ParachainHostImpl::session_info(
      const primitives::BlockHash &block, SessionIndex index) {
    OUTCOME_TRY(ref,
                session_info_.call(
                    *executor_, block, "ParachainHost_session_info", index));
    return *ref;
  }

  outcome::result<std::vector<InboundDownwardMessage>>
  ParachainHostImpl::dmq_contents(const primitives::BlockHash &block,
                                  ParachainId id) {
    OUTCOME_TRY(ref,
                dmq_contents_.call(
                    *executor_, block, "ParachainHost_dmq_contents", id));
    return *ref;
  }

  outcome::result<std::map<ParachainId, std::vector<InboundHrmpMessage>>>
  ParachainHostImpl::inbound_hrmp_channels_contents(
      const primitives::BlockHash &block, ParachainId id) {
    OUTCOME_TRY(ref,
                inbound_hrmp_channels_contents_.call(
                    *executor_,
                    block,
                    "ParachainHost_inbound_hrmp_channels_contents",
                    id));
    return *ref;
  }

  bool ParachainHostImpl::prepare() {
    chain_sub_.onDeactivate(
        [wptr{weak_from_this()}](
            const primitives::events::RemoveAfterFinalizationParams &event) {
          if (auto self = wptr.lock()) {
            self->clearCaches(event);
          }
        });

    return false;
  }

  void ParachainHostImpl::clearCaches(
      const std::vector<primitives::BlockHash> &blocks) {
    active_parachains_.erase(blocks);
    parachain_head_.erase(blocks);
    parachain_code_.erase(blocks);
    validators_.erase(blocks);
    validator_groups_.erase(blocks);
    availability_cores_.erase(blocks);
    session_index_for_child_.erase(blocks);
    candidate_pending_availability_.erase(blocks);
    candidate_events_.erase(blocks);
    session_info_.erase(blocks);
    dmq_contents_.erase(blocks);
    inbound_hrmp_channels_contents_.erase(blocks);
    disabled_validators_.erase(blocks);
  }

  outcome::result<std::optional<std::vector<ExecutorParam>>>
  ParachainHostImpl::session_executor_params(const primitives::BlockHash &block,
                                             SessionIndex idx) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<std::vector<ExecutorParam>>>(
        ctx, "ParachainHost_session_executor_params", idx);
  }

  outcome::result<std::optional<dispute::ScrapedOnChainVotes>>
  ParachainHostImpl::on_chain_votes(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<dispute::ScrapedOnChainVotes>>(
        ctx, "ParachainHost_on_chain_votes");
  }

  outcome::result<std::vector<std::tuple<dispute::SessionIndex,
                                         dispute::CandidateHash,
                                         dispute::DisputeState>>>
  ParachainHostImpl::disputes(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::vector<std::tuple<dispute::SessionIndex,
                                                  dispute::CandidateHash,
                                                  dispute::DisputeState>>>(
        ctx, "ParachainHost_disputes");  // TODO ensure if it works
  }

  outcome::result<std::vector<ValidationCodeHash>>
  ParachainHostImpl::pvfs_require_precheck(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::vector<ValidationCodeHash>>(
        ctx, "ParachainHost_pvfs_require_precheck");
  }

  outcome::result<void> ParachainHostImpl::submit_pvf_check_statement(
      const primitives::BlockHash &block,
      const parachain::PvfCheckStatement &statement,
      const parachain::Signature &signature) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<void>(
        ctx, "ParachainHost_submit_pvf_check_statement", statement, signature);
  }

  outcome::result<std::optional<parachain::fragment::BackingState>>
  ParachainHostImpl::staging_para_backing_state(
      const primitives::BlockHash &block, ParachainId id) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<parachain::fragment::BackingState>>(
        ctx, "ParachainHost_para_backing_state", id);
  }

  outcome::result<parachain::fragment::AsyncBackingParams>
  ParachainHostImpl::staging_async_backing_params(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<parachain::fragment::AsyncBackingParams>(
        ctx, "ParachainHost_async_backing_params");
  }

  outcome::result<uint32_t> ParachainHostImpl::minimum_backing_votes(
      const primitives::BlockHash &block, SessionIndex index) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<uint32_t>(ctx,
                                     "ParachainHost_minimum_backing_votes");
  }

  outcome::result<std::vector<ValidatorIndex>>
  ParachainHostImpl::disabled_validators(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    auto res = executor_->call<std::vector<ValidatorIndex>>(
        ctx, "ParachainHost_disabled_validators");
    if (res.has_error()
        and res.error() == RuntimeExecutionError::EXPORT_FUNCTION_NOT_FOUND) {
      return outcome::success(std::vector<ValidatorIndex>{});
    }
    return res;
  }

  outcome::result<std::optional<ParachainHost::NodeFeatures>>
  ParachainHostImpl::node_features(const primitives::BlockHash &block,
                                   SessionIndex index) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    auto res = executor_->call<ParachainHost::NodeFeatures>(
        ctx, "ParachainHost_node_features");
    if (res.has_error()
        and res.error() == RuntimeExecutionError::EXPORT_FUNCTION_NOT_FOUND) {
      return outcome::success(std::nullopt);
    }
    return res.value();
  }

}  // namespace kagome::runtime
