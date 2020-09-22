/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_SYNCINGROUNDOBSERVER
#define KAGOME_CONSENSUS_GRANDPA_SYNCINGROUNDOBSERVER

#include "consensus/grandpa/grandpa_observer.hpp"

#include "common/logger.hpp"
#include "consensus/grandpa/environment.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Observer of grandpa messages for syncing nodes
   */
  class SyncingGrandpaObserver : public GrandpaObserver {
   public:
    ~SyncingGrandpaObserver() override = default;
    explicit SyncingGrandpaObserver(std::shared_ptr<Environment> environment);

    void onFinalize(const libp2p::peer::PeerId &peer_id, const Fin &f) override;

    void onVoteMessage(const libp2p::peer::PeerId &peer_id,
                       const VoteMessage &msg) override;

    void onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                          const network::CatchUpRequest &msg) override;

    void onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                           const network::CatchUpResponse &msg) override;

   private:
    std::shared_ptr<Environment> environment_;
    common::Logger logger_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_SYNCINGROUNDOBSERVER
