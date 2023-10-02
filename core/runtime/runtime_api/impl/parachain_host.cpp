/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/parachain_host.hpp"

#include "common/blob.hpp"
#include "runtime/executor.hpp"
#include "runtime/runtime_api/impl/parachain_host_types_serde.hpp"

namespace kagome::runtime {

  ParachainHostImpl::ParachainHostImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<DutyRoster> ParachainHostImpl::duty_roster(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<DutyRoster>(ctx, "ParachainHost_duty_roster");
  }

  outcome::result<std::vector<ParachainId>>
  ParachainHostImpl::active_parachains(const primitives::BlockHash &block) {
    OUTCOME_TRY(ref,
                active_parachains_.get_else(
                    block, [&]() -> outcome::result<std::vector<ParachainId>> {
                      OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
                      return executor_->call<std::vector<ParachainId>>(
                          ctx, "ParachainHost_active_parachains");
                    }));
    return *ref;
  }

  outcome::result<std::optional<common::Buffer>>
  ParachainHostImpl::parachain_head(const primitives::BlockHash &block,
                                    ParachainId id) {
    OUTCOME_TRY(
        ref,
        parachain_head_.get_else(
            block, [&]() -> outcome::result<std::optional<common::Buffer>> {
              OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
              return executor_->call<std::optional<Buffer>>(
                  ctx, "ParachainHost_parachain_head", id);
            }));
    return *ref;
  }

  outcome::result<std::optional<common::Buffer>>
  ParachainHostImpl::parachain_code(const primitives::BlockHash &block,
                                    ParachainId id) {
    OUTCOME_TRY(ref,
                parachain_code_.get_else(
                    std::tie(block, id),
                    [&]() -> outcome::result<std::optional<common::Buffer>> {
                      OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
                      return executor_->call<std::optional<common::Buffer>>(
                          ctx, "ParachainHost_parachain_code", id);
                    }));
    return *ref;
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ref,
                validators_.get_else(
                    block, [&]() -> outcome::result<std::vector<ValidatorId>> {
                      OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
                      return executor_->call<std::vector<ValidatorId>>(
                          ctx, "ParachainHost_validators");
                    }));
    return *ref;
  }

  outcome::result<ValidatorGroupsAndDescriptor>
  ParachainHostImpl::validator_groups(const primitives::BlockHash &block) {
    OUTCOME_TRY(
        ref,
        validator_groups_.get_else(
            block, [&]() -> outcome::result<ValidatorGroupsAndDescriptor> {
              OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
              return executor_->call<ValidatorGroupsAndDescriptor>(
                  ctx, "ParachainHost_validator_groups");
            }));
    return *ref;
  }

  outcome::result<std::vector<CoreState>> ParachainHostImpl::availability_cores(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ref,
                availability_cores_.get_else(
                    block, [&]() -> outcome::result<std::vector<CoreState>> {
                      OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
                      return executor_->call<std::vector<CoreState>>(
                          ctx, "ParachainHost_availability_cores");
                    }));
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
    OUTCOME_TRY(ref,
                session_index_for_child_.get_else(
                    block, [&]() -> outcome::result<SessionIndex> {
                      OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
                      return executor_->call<SessionIndex>(
                          ctx, "ParachainHost_session_index_for_child");
                    }));
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
    OUTCOME_TRY(ref,
                validation_code_by_hash_.get_else(
                    std::tie(block, hash),
                    [&]() -> outcome::result<std::optional<ValidationCode>> {
                      OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
                      return executor_->call<std::optional<ValidationCode>>(
                          ctx, "ParachainHost_validation_code_by_hash", hash);
                    }));
    return *ref;
  }

  outcome::result<std::optional<CommittedCandidateReceipt>>
  ParachainHostImpl::candidate_pending_availability(
      const primitives::BlockHash &block, ParachainId id) {
    OUTCOME_TRY(
        ref,
        candidate_pending_availability_.get_else(
            std::tie(block, id),
            [&]() -> outcome::result<std::optional<CommittedCandidateReceipt>> {
              OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
              return executor_->call<std::optional<CommittedCandidateReceipt>>(
                  ctx, "ParachainHost_candidate_pending_availability", id);
            }));
    return *ref;
  }

  outcome::result<std::vector<CandidateEvent>>
  ParachainHostImpl::candidate_events(const primitives::BlockHash &block) {
    OUTCOME_TRY(
        ref,
        candidate_events_.get_else(
            block, [&]() -> outcome::result<std::vector<CandidateEvent>> {
              OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
              return executor_->call<std::vector<CandidateEvent>>(
                  ctx, "ParachainHost_candidate_events");
            }));
    return *ref;
  }

  outcome::result<std::optional<SessionInfo>> ParachainHostImpl::session_info(
      const primitives::BlockHash &block, SessionIndex index) {
    OUTCOME_TRY(ref,
                session_info_.get_else(
                    std::tie(block, index),
                    [&]() -> outcome::result<std::optional<SessionInfo>> {
                      OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
                      return executor_->call<std::optional<SessionInfo>>(
                          ctx, "ParachainHost_session_info", index);
                    }));
    return *ref;
  }

  outcome::result<std::vector<InboundDownwardMessage>>
  ParachainHostImpl::dmq_contents(const primitives::BlockHash &block,
                                  ParachainId id) {
    OUTCOME_TRY(
        ref,
        dmq_contents_.get_else(
            std::tie(block, id),
            [&]() -> outcome::result<std::vector<InboundDownwardMessage>> {
              OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
              return executor_->call<std::vector<InboundDownwardMessage>>(
                  ctx, "ParachainHost_dmq_contents", id);
            }));
    return *ref;
  }

  outcome::result<std::map<ParachainId, std::vector<InboundHrmpMessage>>>
  ParachainHostImpl::inbound_hrmp_channels_contents(
      const primitives::BlockHash &block, ParachainId id) {
    OUTCOME_TRY(
        ref,
        inbound_hrmp_channels_contents_.get_else(
            std::tie(block, id),
            [&]() -> outcome::result<
                      std::map<ParachainId, std::vector<InboundHrmpMessage>>> {
              OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
              return executor_->call<
                  std::map<ParachainId, std::vector<InboundHrmpMessage>>>(
                  ctx, "ParachainHost_inbound_hrmp_channels_contents", id);
            }));
    return *ref;
  }

  bool ParachainHostImpl::prepare() {
    chain_sub_ = std::make_shared<primitives::events::ChainEventSubscriber>(
        peer_view_->intoChainEventsEngine());
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
    for (auto &block : blocks) {
      auto by_block = [&](auto &key, auto &) {
        return std::get<0>(key) == block;
      };

      active_parachains_.erase(block);
      parachain_head_.erase(block);
      parachain_code_.erase_if(by_block);
      validators_.erase(block);
      validator_groups_.erase(block);
      availability_cores_.erase(block);
      session_index_for_child_.erase(block);
      validation_code_by_hash_.erase_if(by_block);
      candidate_pending_availability_.erase_if(by_block);
      candidate_events_.erase(block);
      session_info_.erase_if(by_block);
      dmq_contents_.erase_if(by_block);
      inbound_hrmp_channels_contents_.erase_if(by_block);
    }
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

}  // namespace kagome::runtime
