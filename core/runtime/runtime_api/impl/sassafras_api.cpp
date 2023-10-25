/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/sassafras_api.hpp"

#include "runtime/executor.hpp"

namespace kagome::runtime {

  using namespace consensus;
  using namespace sassafras;
  using namespace bandersnatch;

  SassafrasApiImpl::SassafrasApiImpl(std::shared_ptr<Executor> executor)
      : executor_{std::move(executor)} {
    BOOST_ASSERT(executor_);
  }

  outcome::result<std::optional<RingContext>> SassafrasApiImpl::ring_context(
      const primitives::BlockHash &block) {
    return executor_->callAt<std::optional<RingContext>>(
        block, "SassafrasApi_ring_context");
  }

  outcome::result<bool> SassafrasApiImpl::submit_tickets_unsigned_extrinsic(
      const primitives::BlockHash &block,
      const std::vector<TicketEnvelope> &tickets) {
    return executor_->callAt<bool>(
        block, "SassafrasApi_submit_tickets_unsigned_extrinsic", tickets);
  }

  outcome::result<std::optional<TicketId>> SassafrasApiImpl::slot_ticket_id(
      const primitives::BlockHash &block, SlotNumber slot) {
    return executor_->callAt<std::optional<TicketId>>(
        block, "SassafrasApi_slot_ticket_id", slot);
  }

  outcome::result<std::optional<std::tuple<TicketId, TicketBody>>>
  SassafrasApiImpl::slot_ticket(const primitives::BlockHash &block,
                                consensus::SlotNumber slot) {
    return executor_->callAt<std::optional<std::tuple<TicketId, TicketBody>>>(
        block, "SassafrasApi_slot_ticket", slot);
  }

  outcome::result<Epoch> SassafrasApiImpl::current_epoch(
      const primitives::BlockHash &block) {
    return executor_->callAt<Epoch>(block, "SassafrasApi_current_epoch");
  }

  outcome::result<Epoch> SassafrasApiImpl::next_epoch(
      const primitives::BlockHash &block) {
    return executor_->callAt<Epoch>(block, "SassafrasApi_next_epoch");
  }

  outcome::result<std::optional<OpaqueKeyOwnershipProof>>
  SassafrasApiImpl::generate_key_ownership_proof(
      const primitives::BlockHash &block,
      const primitives::AuthorityId &authority_id) {
    return executor_->callAt<std::optional<OpaqueKeyOwnershipProof>>(
        block, "SassafrasApi_generate_key_ownership_proof", authority_id);
  }

  outcome::result<bool>
  SassafrasApiImpl::submit_report_equivocation_unsigned_extrinsic(
      const primitives::BlockHash &block,
      const EquivocationProof &equivocation_proof,
      const OpaqueKeyOwnershipProof &key_owner_proof) {
    return executor_->callAt<bool>(
        block,
        "SassafrasApi_submit_report_equivocation_unsigned_extrinsic",
        equivocation_proof,
        key_owner_proof);
  }

}  // namespace kagome::runtime
