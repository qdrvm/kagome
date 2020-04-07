/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/syncing_round_observer.hpp"

namespace kagome::consensus::grandpa {

  SyncingRoundObserver::SyncingRoundObserver(
      std::shared_ptr<consensus::grandpa::Environment> environment)
      : environment_{std::move(environment)},
        logger_{common::createLogger("SyncingRoundObserver")} {}

  void SyncingRoundObserver::onVoteMessage(const VoteMessage &msg) {
    // do nothing
  }

  void SyncingRoundObserver::onFinalize(const Fin &f) {
    if (auto fin_res =
            environment_->finalize(f.vote.block_hash, f.justification);
        not fin_res) {
      logger_->error("Could not finalize: {}", fin_res.error().message());
    }
    logger_->info("Finalized block number {} with hash {}",
                  f.vote.block_number,
                  f.vote.block_hash.toHex());
  }

}  // namespace kagome::consensus::grandpa
