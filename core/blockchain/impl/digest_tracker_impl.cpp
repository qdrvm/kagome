/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "digest_tracker_impl.hpp"

#include "common/visitor.hpp"
#include "consensus/babe/babe_digest_observer.hpp"
#include "consensus/grandpa/grandpa_digest_observer.hpp"

namespace kagome::blockchain {

  DigestTrackerImpl::DigestTrackerImpl(
      std::shared_ptr<consensus::babe::BabeDigestObserver> babe_update_observer,
      std::shared_ptr<consensus::grandpa::GrandpaDigestObserver>
          grandpa_digest_observer)
      : babe_digest_observer_(std::move(babe_update_observer)),
        grandpa_digest_observer_(std::move(grandpa_digest_observer)),
        logger_(log::createLogger("DigestTracker", "digest_tracker")) {
    BOOST_ASSERT(babe_digest_observer_ != nullptr);
    BOOST_ASSERT(grandpa_digest_observer_ != nullptr);
  }

  outcome::result<void> DigestTrackerImpl::onDigest(
      const primitives::BlockInfo &block, const primitives::Digest &digest) {
    SL_TRACE(logger_, "Start process digest on block {}", block);
    for (auto &item : digest) {
      auto res = visit_in_place(
          item,
          [&](const primitives::Consensus &item) {
            SL_TRACE(logger_,
                     "Consensus-digest on block {}, engine '{}'",
                     block,
                     item.consensus_engine_id.toString());
            return onConsensus(block, item);
          },
          [&](const primitives::Seal &item) {
            SL_TRACE(logger_,
                     "Seal-digest on block {}, engine '{}'",
                     block,
                     item.consensus_engine_id.toString());
            return outcome::success();  // It does not processed by tracker
          },
          [&](const primitives::PreRuntime &item) {
            SL_TRACE(logger_,
                     "PreRuntime-digest on block {}, engine '{}'",
                     block,
                     item.consensus_engine_id.toString());
            return onPreRuntime(block, item);
          },
          [&](const primitives::RuntimeEnvironmentUpdated &item) {
            SL_TRACE(
                logger_, "RuntimeEnvironmentUpdated-digest on block {}", block);
            return outcome::success();  // It does not processed by tracker
          },
          [&](const auto &) {
            SL_WARN(logger_,
                    "Unsupported digest on block {}: variant #{}",
                    block,
                    item.which());
            return outcome::success();
          });
      OUTCOME_TRY(res);
    }
    SL_TRACE(logger_, "End process digest on block {}", block);
    return outcome::success();
  }

  void DigestTrackerImpl::cancel(const primitives::BlockInfo &block) {
    // Cancel tracked babe digest
    babe_digest_observer_->cancel(block);

    // Cancel tracked grandpa digest
    grandpa_digest_observer_->cancel(block);
  }

  outcome::result<void> DigestTrackerImpl::onConsensus(
      const primitives::BlockInfo &block,
      const primitives::Consensus &message) {
    if (message.consensus_engine_id == primitives::kBabeEngineId) {
      OUTCOME_TRY(digest, scale::decode<primitives::BabeDigest>(message.data));

      return babe_digest_observer_->onDigest(block, digest);

    } else if (message.consensus_engine_id == primitives::kGrandpaEngineId) {
      OUTCOME_TRY(digest,
                  scale::decode<primitives::GrandpaDigest>(message.data));

      return grandpa_digest_observer_->onDigest(block, digest);

    } else if (message.consensus_engine_id
                   == primitives::kUnsupportedEngineId_BEEF
               or message.consensus_engine_id
                      == primitives::kUnsupportedEngineId_POL1) {
      SL_TRACE(logger_,
               "Unsupported consensus engine id in block {}: {}",
               block,
               message.consensus_engine_id.toString());
      return outcome::success();

    } else {
      SL_WARN(logger_,
              "Unknown consensus engine id in block {}: {}",
              block,
              message.consensus_engine_id.toString());
      return outcome::success();
    }
  }

  outcome::result<void> DigestTrackerImpl::onPreRuntime(
      const primitives::BlockInfo &block,
      const primitives::PreRuntime &message) {
    if (message.consensus_engine_id == primitives::kBabeEngineId) {
      OUTCOME_TRY(
          digest,
          scale::decode<consensus::babe::BabeBlockHeader>(message.data));

      return babe_digest_observer_->onDigest(block, digest);
    } else {
      SL_WARN(logger_,
              "Unknown consensus engine id in block {}: {}",
              block,
              message.consensus_engine_id.toString());
      return outcome::success();
    }
  }

}  // namespace kagome::blockchain
