/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_WARP_CACHE_HPP
#define KAGOME_NETWORK_WARP_CACHE_HPP

#include "application/app_state_manager.hpp"
#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "log/logger.hpp"
#include "network/warp/types.hpp"
#include "primitives/event_types.hpp"
#include "storage/map_prefix/prefix.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::network {
  /**
   * Caches number/hash of blocks with grandpa scheduled/forced change digest.
   * Generates warp sync proof.
   */
  class WarpSyncCache : public std::enable_shared_from_this<WarpSyncCache> {
   public:
    enum Error {
      NOT_FINALIZED = 1,
      NOT_IN_CHAIN,
    };

    WarpSyncCache(
        application::AppStateManager &app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::BlockHeaderRepository> block_repository,
        std::shared_ptr<storage::SpacedStorage> db,
        std::shared_ptr<primitives::events::ChainSubscriptionEngine>
            chain_sub_engine);

    outcome::result<WarpSyncProof> getProof(
        const primitives::BlockHash &after_hash) const;

   private:
    outcome::result<void> cacheMore(primitives::BlockNumber finalized);
    outcome::result<void> start(
        std::shared_ptr<primitives::events::ChainSubscriptionEngine>
            chain_sub_engine);

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockHeaderRepository> block_repository_;
    mutable storage::MapPrefix db_prefix_;
    log::Logger log_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;
    primitives::BlockNumber cache_next_ = 0;
  };
}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, WarpSyncCache::Error)

#endif  // KAGOME_NETWORK_WARP_CACHE_HPP
