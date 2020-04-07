/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_IMPL_SYNCING_BABE_OBSERVER_HPP
#define KAGOME_CORE_CONSENSUS_BABE_IMPL_SYNCING_BABE_OBSERVER_HPP

#include "network/babe_observer.hpp"

#include "consensus/babe/impl/block_executor.hpp"

namespace kagome::consensus {

  class SyncingBabeObserver : public network::BabeObserver {
   public:
    ~SyncingBabeObserver() override = default;

    explicit SyncingBabeObserver(
        std::shared_ptr<consensus::BlockExecutor> block_executor);

    void onBlockAnnounce(const network::BlockAnnounce &announce) override;

   private:
    std::shared_ptr<consensus::BlockExecutor> block_executor_;
  };

}  // namespace kagome::consensus

#endif  // KAGOME_CORE_CONSENSUS_BABE_IMPL_SYNCING_BABE_OBSERVER_HPP
