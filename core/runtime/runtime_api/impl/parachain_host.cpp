/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/parachain_host.hpp"

#include <boost/asio/execution/context.hpp>

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
        chain_events_engine_{std::move(chain_events_engine)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<std::vector<ParachainId>>
  ParachainHostImpl::active_parachains(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::vector<ParachainId>>(
          ctx, "ParachainHost_active_parachains");
  }

  outcome::result<std::optional<common::Buffer>>
  ParachainHostImpl::parachain_head(const primitives::BlockHash &block,
                                    ParachainId id) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<common::Buffer>>(
            ctx, "ParachainHost_parachain_head", id);
  }

  outcome::result<std::optional<common::Buffer>>
  ParachainHostImpl::parachain_code(const primitives::BlockHash &block,
                                    ParachainId id) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<common::Buffer>>(
            ctx, "ParachainHost_parachain_code", id);
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::vector<ValidatorId>>(
            ctx, "ParachainHost_validators");
  }

  outcome::result<ValidatorGroupsAndDescriptor>
  ParachainHostImpl::validator_groups(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<ValidatorGroupsAndDescriptor>(
            ctx, "ParachainHost_validator_groups");
  }

  outcome::result<std::vector<CoreState>> ParachainHostImpl::availability_cores(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::vector<CoreState>>(
            ctx, "ParachainHost_availability_cores");
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
      return executor_->call<std::optional<ValidationCode>>(
          ctx, "ParachainHost_validation_code_by_hash", hash);

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
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<CommittedCandidateReceipt>>(
          ctx, "ParachainHost_candidate_pending_availability", id);
  }

  outcome::result<std::vector<CandidateEvent>>
  ParachainHostImpl::candidate_events(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::vector<CandidateEvent>>(
            ctx, "ParachainHost_candidate_events");
  }

  outcome::result<std::optional<SessionInfo>> ParachainHostImpl::session_info(
      const primitives::BlockHash &block, SessionIndex index) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<SessionInfo>>(
        ctx, "ParachainHost_session_info", index);
  }

  outcome::result<std::vector<InboundDownwardMessage>>
  ParachainHostImpl::dmq_contents(const primitives::BlockHash &block,
                                  ParachainId id) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
        return executor_->call<std::vector<InboundDownwardMessage>>(
                ctx, "ParachainHost_dmq_contents", id);
  }

  outcome::result<std::map<ParachainId, std::vector<InboundHrmpMessage>>>
  ParachainHostImpl::inbound_hrmp_channels_contents(
      const primitives::BlockHash &block, ParachainId id) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::map<ParachainId, std::vector<InboundHrmpMessage>>>(
            ctx, "ParachainHost_inbound_hrmp_channels_contents", id);
  }

  bool ParachainHostImpl::prepare() {
    chain_sub_ = std::make_shared<primitives::events::ChainEventSubscriber>(
        chain_events_engine_);
    chain_sub_->subscribe(
        chain_sub_->generateSubscriptionSetId(),
        primitives::events::ChainEventType::kDeactivateAfterFinalization);
    chain_sub_->setCallback([wptr{weak_from_this()}](
                                auto /*set_id*/,
                                auto && /*internal_obj*/,
                                auto /*event_type*/,
                                const primitives::events::ChainEventParams
                                    &event) {
      if (auto self = wptr.lock()) {
        auto event_opt =
            if_type<const primitives::events::RemoveAfterFinalizationParams>(
                event);
        if (event_opt.has_value()) {
          self->clearCaches(event_opt.value());
        }
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

}  // namespace kagome::runtime
