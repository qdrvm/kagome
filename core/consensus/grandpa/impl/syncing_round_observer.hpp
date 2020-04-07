/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_SYNCING_ROUND_OBSERVER_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_SYNCING_ROUND_OBSERVER_HPP

#include "consensus/grandpa/round_observer.hpp"

#include "consensus/grandpa/environment.hpp"
#include "common/logger.hpp"

namespace kagome::consensus::grandpa {

  class SyncingRoundObserver : public RoundObserver {
   public:
    ~SyncingRoundObserver() override = default;
    explicit SyncingRoundObserver(std::shared_ptr<Environment> environment);

    void onFinalize(const Fin &f) override;

    void onVoteMessage(const VoteMessage &msg) override;

   private:
    std::shared_ptr<Environment> environment_;
    common::Logger logger_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_SYNCING_ROUND_OBSERVER_HPP
