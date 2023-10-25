/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "digest_tracker_impl.hpp"

#include "common/visitor.hpp"
#include "consensus/grandpa/grandpa_digest_observer.hpp"

namespace kagome::blockchain {

  DigestTrackerImpl::DigestTrackerImpl(
      std::shared_ptr<consensus::grandpa::GrandpaDigestObserver>
          grandpa_digest_observer)
      : grandpa_digest_observer_(std::move(grandpa_digest_observer)),
        logger_(log::createLogger("DigestTracker", "digest_tracker")) {
    BOOST_ASSERT(grandpa_digest_observer_ != nullptr);
  }

  outcome::result<void> DigestTrackerImpl::onDigest(
      const primitives::BlockContext &context,
      const primitives::Digest &digest) {
    SL_TRACE(logger_, "Start process digest on block {}", context.block_info);
    for (auto &item : digest) {
      auto res = visit_in_place(
          item,
          [&](const primitives::Consensus &item) {
            SL_TRACE(logger_,
                     "Consensus-digest on block {}, engine '{}'",
                     context.block_info,
                     item.consensus_engine_id.toString());
            return onConsensus(context, item);
          },
          [&](const primitives::Seal &item) {
            SL_TRACE(logger_,
                     "Seal-digest on block {}, engine '{}'",
                     context.block_info,
                     item.consensus_engine_id.toString());
            return outcome::success();  // It does not processed by tracker
          },
          [&](const primitives::PreRuntime &item) {
            return outcome::success();
          },
          [&](const primitives::RuntimeEnvironmentUpdated &item) {
            SL_TRACE(logger_,
                     "RuntimeEnvironmentUpdated-digest on block {}",
                     context.block_info);
            return outcome::success();  // It does not processed by tracker
          },
          [&](const auto &) {
            SL_WARN(logger_,
                    "Unsupported digest on block {}: variant #{}",
                    context.block_info,
                    item.which());
            return outcome::success();
          });
      OUTCOME_TRY(res);
    }
    SL_TRACE(logger_, "End process digest on block {}", context.block_info);
    return outcome::success();
  }

  void DigestTrackerImpl::cancel(const primitives::BlockInfo &block) {
    // Cancel tracked grandpa digest
    grandpa_digest_observer_->cancel(block);
  }

  outcome::result<void> DigestTrackerImpl::onConsensus(
      const primitives::BlockContext &context,
      const primitives::Consensus &message) {
    if (message.consensus_engine_id == primitives::kBabeEngineId) {
      OUTCOME_TRY(digest,
                  scale::decode<primitives::BabeDigest>(message.data.view()));

      return outcome::success();

    } else if (message.consensus_engine_id == primitives::kGrandpaEngineId) {
      OUTCOME_TRY(
          digest,
          scale::decode<primitives::GrandpaDigest>(message.data.view()));

      return grandpa_digest_observer_->onDigest(context, digest);

    } else if (message.consensus_engine_id == primitives::kBeefyEngineId) {
      return outcome::success();
    } else if (message.consensus_engine_id
               == primitives::kUnsupportedEngineId_POL1) {
      SL_TRACE(logger_,
               "Unsupported consensus engine id in block {}: {}",
               context.block_info,
               message.consensus_engine_id.toString());
      return outcome::success();

    } else {
      SL_WARN(logger_,
              "Unknown consensus engine id in block {}: {}",
              context.block_info,
              message.consensus_engine_id.toString());
      return outcome::success();
    }
  }
}  // namespace kagome::blockchain
