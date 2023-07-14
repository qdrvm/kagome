/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/participation/impl/participation_impl.hpp"

#include <future>

#include "dispute_coordinator/dispute_coordinator.hpp"
#include "dispute_coordinator/participation/impl/queues_impl.hpp"
#include "dispute_coordinator/participation/queues.hpp"
#include "parachain/availability/recovery/recovery.hpp"
#include "parachain/pvf/pvf.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::dispute {

  ParticipationImpl::ParticipationImpl(
      std::shared_ptr<blockchain::BlockHeaderRepository>
          block_header_repository,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<runtime::ParachainHost> api,
      std::shared_ptr<parachain::Recovery> recovery,
      std::shared_ptr<parachain::Pvf> pvf,
      std::shared_ptr<ThreadHandler> internal_context,
      std::weak_ptr<DisputeCoordinator> dispute_coordinator)
      : block_header_repository_(std::move(block_header_repository)),
        api_(std::move(api)),
        recovery_(std::move(recovery)),
        pvf_(std::move(pvf)),
        internal_context_(std::move(internal_context)),
        dispute_coordinator_(std::move(dispute_coordinator)),
        queue_(std::make_unique<QueuesImpl>(
            block_header_repository_, std::move(hasher), std::move(api))) {
    BOOST_ASSERT(block_header_repository_ != nullptr);
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(recovery_ != nullptr);
    BOOST_ASSERT(pvf_ != nullptr);
    BOOST_ASSERT(internal_context_ != nullptr);
    BOOST_ASSERT(not dispute_coordinator_.expired());
  }

  outcome::result<void> ParticipationImpl::queue_participation(
      ParticipationPriority priority, ParticipationRequest request) {
    // Participation already running - we can ignore that request:
    if (running_participations_.count(request.candidate_hash)) {
      return outcome::success();
    }

    // Available capacity - participate right away (if we already have a recent
    // block):
    if (recent_block_.has_value()) {
      if (running_participations_.size() < kMaxParallelParticipations) {
        return fork_participation(request, recent_block_->hash);
      }
    }

    // Out of capacity/no recent block yet - queue:
    return queue_->queue(priority, request);
  }

  outcome::result<void> ParticipationImpl::fork_participation(
      ParticipationRequest request, primitives::BlockHash recent_head) {
    if (running_participations_.emplace(request.candidate_hash).second) {
      // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/participation/mod.rs#L256
      internal_context_->execute([wp = weak_from_this(),
                                  request{std::move(request)},
                                  recent_head{std::move(recent_head)}]() {
        if (auto self = wp.lock()) {
          self->participate(std::move(request), std::move(recent_head));
        }
      });
    }
    return outcome::success();
  }

  /// Dequeue until `MAX_PARALLEL_PARTICIPATIONS` is reached.
  outcome::result<void> ParticipationImpl::dequeue_until_capacity(
      const primitives::BlockHash &recent_head) {
    while (running_participations_.size() < kMaxParallelParticipations) {
      auto req = queue_->dequeue();
      if (req.has_value()) {
        OUTCOME_TRY(fork_participation(req.value(), recent_head));
      } else {
        break;
      }
    }
    return outcome::success();
  }

  outcome::result<void> ParticipationImpl::process_active_leaves_update(
      const ActiveLeavesUpdate &update) {
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/participation/mod.rs#L197

    if (update.activated.has_value()) {
      const auto &activated = update.activated.value();

      if (not recent_block_.has_value()) {
        recent_block_ = std::make_optional<primitives::BlockInfo>(
            activated.number, activated.hash);
        // Work got potentially unblocked:
        OUTCOME_TRY(dequeue_until_capacity(activated.hash));

      } else if (activated.number > recent_block_->number) {
        recent_block_ = std::make_optional<primitives::BlockInfo>(
            activated.number, activated.hash);
      }
    }

    return outcome::success();
  }

  outcome::result<void> ParticipationImpl::get_participation_result(
      const ParticipationStatement &msg) {
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/participation/mod.rs#L181
    running_participations_.erase(msg.candidate_hash);

    BOOST_ASSERT_MSG(
        recent_block_.has_value(),
        "We never ever reset recent_block to `None` and we already "
        "received a result, so it must have been set before. qed.");
    auto recent_block = recent_block_.value();

    return dequeue_until_capacity(recent_block.hash);
  }

  outcome::result<void> ParticipationImpl::bump_to_priority_for_candidates(
      std::vector<CandidateReceipt> &included_receipts) {
    for (auto &receipt : included_receipts) {
      OUTCOME_TRY(queue_->prioritize_if_present(receipt));
    }
    return outcome::success();
  }

  void ParticipationImpl::participate(ParticipationRequest request,
                                      primitives::BlockHash block_hash) {
    auto ctx = std::make_shared<ParticipationContext>(
        ParticipationContext{std::move(request), std::move(block_hash)});

    participate_stage1(
        ctx, [request, wp = dispute_coordinator_](ParticipationOutcome res) {
          if (auto dispute_coordinator = wp.lock()) {
            dispute_coordinator->onParticipation(ParticipationStatement{
                .session = request.session,
                .candidate_hash = request.candidate_hash,
                .candidate_receipt = request.candidate_receipt,
                .outcome = res,
            });
          }
        });
  }

  void ParticipationImpl::participate_stage1(ParticipationContextPtr ctx,
                                             ParticipationCallback &&cb) {
    // Recovery available data

    // in order to validate a candidate we need to start by recovering the
    // available data
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/participation/mod.rs#L279

    recovery_->recover(
        ctx->request.candidate_receipt,
        ctx->request.session,
        std::nullopt,
        [wp = weak_from_this(), ctx, cb = std::move(cb)](auto res_opt) mutable {
          if (auto self = wp.lock()) {
            if (not res_opt.has_value()) {
              cb(ParticipationOutcome::Unavailable);
              return;
            }
            auto &available_data_res = res_opt.value();
            if (available_data_res.has_error()) {
              cb(ParticipationOutcome::Error);
              return;
            }
            auto &available_data = available_data_res.value();

            // SL_TRACE(
            //   log_,
            //   "Successfully received result of available data recovery: {}",
            //   available_data.size());

            ctx->available_data.emplace(std::move(available_data));
            self->participate_stage2(std::move(ctx), std::move(cb));
          }
        });
  }

  void ParticipationImpl::participate_stage2(ParticipationContextPtr ctx,
                                             ParticipationCallback &&cb) {
    // Fetch validation code

    // we also need to fetch the validation code which we can reference by its
    // hash as taken from the candidate descriptor
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/participation/mod.rs#L312
    auto validation_code_res = api_->validation_code_by_hash(
        ctx->block_hash,
        ctx->request.candidate_receipt.descriptor.validation_code_hash);
    if (validation_code_res.has_error()) {
      // SL_WARN(log_,
      //         "Error when fetching validation code: {}",
      //         validation_code_res.error());
      cb(ParticipationOutcome::Error);
      return;
    }
    auto &validation_code_opt = validation_code_res.value();
    if (not validation_code_opt.has_value()) {
      // SL_WARN(log_,
      //         "Validation code unavailable for code hash {} "
      //         "in the state of block {}",
      //         ctx->request.candidate_receipt.descriptor.validation_code_hash,
      //         ctx->block_hash);
      cb(ParticipationOutcome::Error);
      return;
    }
    ctx->validation_code = std::move(validation_code_opt);
    participate_stage3(std::move(ctx), std::move(cb));
  }

  void ParticipationImpl::participate_stage3(ParticipationContextPtr _ctx,
                                             ParticipationCallback &&_cb) {
    REINVOKE_2(*internal_context_, participate_stage3, _ctx, _cb, ctx, cb);

    // Issue a request to validate the candidate with the provided exhaustive
    // parameters
    //
    // We use the approval execution timeout because this is intended to be run
    // outside of backing and therefore should be subject to the same level of
    // leeway.
    // https://github.com/paritytech/polkadot/blob/40974fb99c86f5c341105b7db53c7aa0df707d66/node/core/dispute-coordinator/src/participation/mod.rs#L346

    BOOST_ASSERT(ctx->available_data.has_value());
    BOOST_ASSERT(ctx->validation_code.has_value());

    auto res = pvf_->pvfValidate(ctx->available_data->validation_data,
                                 ctx->available_data->pov,
                                 ctx->request.candidate_receipt,
                                 ctx->validation_code.value());

    // we cast votes (either positive or negative) depending on the outcome of
    // the validation and if valid, whether the commitments hash matches

    if (res.has_value()) {
      cb(ParticipationOutcome::Valid);
      return;
    }

    // SL_WARN(log_,
    //         "Candidate {} considered invalid: {}",
    //         ctx->request.candidate_hash,
    //         res.error());

    cb(ParticipationOutcome::Invalid);
  }

}  // namespace kagome::dispute
