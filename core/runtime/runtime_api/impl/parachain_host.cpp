/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/parachain_host.hpp"

#include "common/blob.hpp"
#include "runtime/common/executor.hpp"
#include "runtime/runtime_api/impl/parachain_host_types_serde.hpp"

namespace kagome::runtime {

  ParachainHostImpl::ParachainHostImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<DutyRoster> ParachainHostImpl::duty_roster(
      const primitives::BlockHash &block) {
    return executor_->callAt<DutyRoster>(block, "ParachainHost_duty_roster");
  }

  outcome::result<std::vector<ParachainId>>
  ParachainHostImpl::active_parachains(const primitives::BlockHash &block) {
    return executor_->callAt<std::vector<ParachainId>>(
        block, "ParachainHost_active_parachains");
  }

  outcome::result<std::optional<common::Buffer>>
  ParachainHostImpl::parachain_head(const primitives::BlockHash &block,
                                    ParachainId id) {
    return executor_->callAt<std::optional<Buffer>>(
        block, "ParachainHost_parachain_head", id);
  }

  outcome::result<std::optional<common::Buffer>>
  ParachainHostImpl::parachain_code(const primitives::BlockHash &block,
                                    ParachainId id) {
    return executor_->callAt<std::optional<common::Buffer>>(
        block, "ParachainHost_parachain_code", id);
  }

  outcome::result<std::vector<ValidatorId>> ParachainHostImpl::validators(
      const primitives::BlockHash &block) {
    return executor_->callAt<std::vector<ValidatorId>>(
        block, "ParachainHost_validators");
  }

  outcome::result<ValidatorGroupsAndDescriptor>
  ParachainHostImpl::validator_groups(const primitives::BlockHash &block) {
    return executor_->callAt<ValidatorGroupsAndDescriptor>(
        block, "ParachainHost_validator_groups");
  }

  outcome::result<std::vector<CoreState>> ParachainHostImpl::availability_cores(
      const primitives::BlockHash &block) {
    return executor_->callAt<std::vector<CoreState>>(
        block, "ParachainHost_availability_cores");
  }

  outcome::result<std::optional<PersistedValidationData>>
  ParachainHostImpl::persisted_validation_data(
      const primitives::BlockHash &block,
      ParachainId id,
      OccupiedCoreAssumption assumption) {
    return executor_->callAt<std::optional<PersistedValidationData>>(
        block, "ParachainHost_persisted_validation_data", id, assumption);
  }

  outcome::result<bool> ParachainHostImpl::check_validation_outputs(
      const primitives::BlockHash &block,
      ParachainId id,
      CandidateCommitments outputs) {
    return executor_->callAt<bool>(
        block, "ParachainHost_check_validation_outputs", id, outputs);
  }

  outcome::result<SessionIndex> ParachainHostImpl::session_index_for_child(
      const primitives::BlockHash &block) {
    return executor_->callAt<SessionIndex>(
        block, "ParachainHost_session_index_for_child");
  }

  outcome::result<std::optional<ValidationCode>>
  ParachainHostImpl::validation_code(const primitives::BlockHash &block,
                                     ParachainId id,
                                     OccupiedCoreAssumption assumption) {
    return executor_->callAt<std::optional<ValidationCode>>(
        block, "ParachainHost_validation_code", id, assumption);
  }

  outcome::result<std::optional<ValidationCode>>
  ParachainHostImpl::validation_code_by_hash(const primitives::BlockHash &block,
                                             ValidationCodeHash hash) {
    return executor_->callAt<std::optional<ValidationCode>>(
        block, "ParachainHost_validation_code_by_hash", hash);
  }

  outcome::result<std::optional<CommittedCandidateReceipt>>
  ParachainHostImpl::candidate_pending_availability(
      const primitives::BlockHash &block, ParachainId id) {
    return executor_->callAt<std::optional<CommittedCandidateReceipt>>(
        block, "ParachainHost_candidate_pending_availability", id);
  }

  outcome::result<std::vector<CandidateEvent>>
  ParachainHostImpl::candidate_events(const primitives::BlockHash &block) {
    return executor_->callAt<std::vector<CandidateEvent>>(
        block, "ParachainHost_candidate_events");
  }

  outcome::result<std::optional<SessionInfo>> ParachainHostImpl::session_info(
      const primitives::BlockHash &block, SessionIndex index) {
    return executor_->callAt<std::optional<SessionInfo>>(
        block, "ParachainHost_session_info", index);
  }

  outcome::result<std::vector<InboundDownwardMessage>>
  ParachainHostImpl::dmq_contents(const primitives::BlockHash &block,
                                  ParachainId id) {
    return executor_->callAt<std::vector<InboundDownwardMessage>>(
        block, "ParachainHost_dmq_contents", id);
  }

  outcome::result<std::map<ParachainId, std::vector<InboundHrmpMessage>>>
  ParachainHostImpl::inbound_hrmp_channels_contents(
      const primitives::BlockHash &block, ParachainId id) {
    return executor_
        ->callAt<std::map<ParachainId, std::vector<InboundHrmpMessage>>>(
            block, "ParachainHost_inbound_hrmp_channels_contents", id);
  }

}  // namespace kagome::runtime
