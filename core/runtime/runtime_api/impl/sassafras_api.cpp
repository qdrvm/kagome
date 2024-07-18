/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/sassafras_api.hpp"

#include "runtime/common/runtime_execution_error.hpp"
#include "runtime/executor.hpp"

namespace kagome::runtime {

  SassafrasApiImpl::SassafrasApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<std::optional<crypto::bandersnatch::vrf::RingContext>>
  SassafrasApiImpl::ring_context(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_
        ->call<std::optional<crypto::bandersnatch::vrf::RingContext>>(
            ctx, "SassafrasApi_ring_context");
  }

  outcome::result<bool> SassafrasApiImpl::submit_tickets_unsigned_extrinsic(
      const primitives::BlockHash &block,
      const std::vector<consensus::sassafras::TicketEnvelope> &tickets) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<bool>(
        ctx, "SassafrasApi_submit_tickets_unsigned_extrinsic", tickets);
  }

  outcome::result<std::optional<consensus::sassafras::TicketId>>
  SassafrasApiImpl::slot_ticket_id(const primitives::BlockHash &block,
                                   consensus::SlotNumber slot) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<std::optional<consensus::sassafras::TicketId>>(
        ctx, "SassafrasApi_slot_ticket_id", slot);
  }

  outcome::result<std::optional<std::tuple<consensus::sassafras::TicketId,
                                           consensus::sassafras::TicketBody>>>
  SassafrasApiImpl::slot_ticket(const primitives::BlockHash &block,
                                consensus::SlotNumber slot) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_
        ->call<std::optional<std::tuple<consensus::sassafras::TicketId,
                                        consensus::sassafras::TicketBody>>>(
            ctx, "SassafrasApi_slot_ticket", slot);
  }

  outcome::result<consensus::sassafras::Epoch> SassafrasApiImpl::current_epoch(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<consensus::sassafras::Epoch>(
        ctx, "SassafrasApi_current_epoch");
  }

  outcome::result<consensus::sassafras::Epoch> SassafrasApiImpl::next_epoch(
      const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<consensus::sassafras::Epoch>(
        ctx, "SassafrasApi_next_epoch");
  }

  outcome::result<std::optional<consensus::sassafras::OpaqueKeyOwnershipProof>>
  SassafrasApiImpl::generate_key_ownership_proof(
      const primitives::BlockHash &block,
      const consensus::sassafras::AuthorityId &authority_id) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_
        ->call<std::optional<consensus::sassafras::OpaqueKeyOwnershipProof>>(
            ctx, "SassafrasApi_generate_key_ownership_proof", authority_id);
  }

  outcome::result<void>
  SassafrasApiImpl::submit_report_equivocation_unsigned_extrinsic(
      const primitives::BlockHash &block,
      const consensus::sassafras::EquivocationProof &equivocation_proof,
      const consensus::sassafras::OpaqueKeyOwnershipProof &key_owner_proof) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    return executor_->call<void>(
        ctx,
        "SassafrasApi_submit_report_equivocation_unsigned_extrinsic",
        equivocation_proof,
        key_owner_proof);
  }

  outcome::result<std::vector<consensus::AuthorityIndex>>
  SassafrasApiImpl::disabled_validators(const primitives::BlockHash &block) {
    OUTCOME_TRY(ctx, executor_->ctx().ephemeralAt(block));
    auto res = executor_->call<std::vector<consensus::AuthorityIndex>>(
        ctx, "Sassafras_disabled_validators");
    if (res.has_error()
        and res.error() == RuntimeExecutionError::EXPORT_FUNCTION_NOT_FOUND) {
      return std::vector<consensus::AuthorityIndex>{};
    }
    return res;
  }
}  // namespace kagome::runtime
