/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/syncing_grandpa_observer.hpp"

namespace kagome::consensus::grandpa {

  SyncingGrandpaObserver::SyncingGrandpaObserver(
      std::shared_ptr<consensus::grandpa::Environment> environment)
      : environment_{std::move(environment)},
        logger_{common::createLogger("SyncingGrandpaObserver")} {}

  void SyncingGrandpaObserver::onVoteMessage(
      const libp2p::peer::PeerId &peer_id, const VoteMessage &msg) {
    // do nothing as syncing node does not care about vote messages
  }

  void SyncingGrandpaObserver::onFinalize(const libp2p::peer::PeerId &peer_id,
                                          const Fin &f) {
    if (auto fin_res =
            environment_->finalize(f.vote.block_hash, f.justification);
        not fin_res) {
      logger_->error("Could not finalize block with hash {}. Reason: {}",
                     f.vote.block_hash.toHex(),
                     fin_res.error().message());
      return;
    }
  }

  void SyncingGrandpaObserver::onCatchUpRequest(
      const libp2p::peer::PeerId &peer_id, const network::CatchUpRequest &msg) {
    // do nothing as syncing node does not care about catch-up messages
  }

  void SyncingGrandpaObserver::onCatchUpResponse(
      const libp2p::peer::PeerId &peer_id,
      const network::CatchUpResponse &msg) {
    // do nothing as syncing node does not care about catch-up messages
  }

}  // namespace kagome::consensus::grandpa
