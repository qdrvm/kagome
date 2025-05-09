/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "log/logger.hpp"
#include "network/warp/types.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::application {
  class AppStateManager;
}  // namespace kagome::application

namespace kagome::blockchain {
  class BlockTree;
  class BlockStorage;
}  // namespace kagome::blockchain

namespace kagome::consensus::babe {
  class BabeConfigRepository;
}  // namespace kagome::consensus::babe

namespace kagome::consensus::grandpa {
  class IVerifiedJustificationQueue;
  class JustificationObserver;
  class AuthorityManager;
}  // namespace kagome::consensus::grandpa

namespace kagome::crypto {
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::network {
  class WarpSyncCache;
}  // namespace kagome::network

namespace kagome::storage {
  class SpacedStorage;
}  // namespace kagome::storage

namespace kagome::network {
  using consensus::grandpa::GrandpaJustification;
  using primitives::BlockHeader;

  /**
   * Applies warp sync changes to other components.
   * Recovers when process was restarted.
   */
  class WarpSync : public std::enable_shared_from_this<WarpSync> {
   public:
    struct Op {
      primitives::BlockInfo block_info;
      primitives::BlockHeader header;
      consensus::grandpa::GrandpaJustification justification;
      consensus::grandpa::AuthoritySet authorities;
    };

    WarpSync(
        application::AppStateManager &app_state_manager,
        std::shared_ptr<crypto::Hasher> hasher,
        storage::SpacedStorage &db,
        std::shared_ptr<consensus::grandpa::JustificationObserver> grandpa,
        std::shared_ptr<blockchain::BlockStorage> block_storage,
        std::shared_ptr<network::WarpSyncCache> warp_sync_cache,
        std::shared_ptr<consensus::grandpa::AuthorityManager> authority_manager,
        std::shared_ptr<consensus::grandpa::IVerifiedJustificationQueue>
            verified_justification_queue,
        std::shared_ptr<consensus::babe::BabeConfigRepository>
            babe_config_repository,
        std::shared_ptr<blockchain::BlockTree> block_tree);

    /**
     * `AppStateManager::atLaunch` hook
     */
    void start();

    /**
     * @return next request to send
     */
    std::optional<primitives::BlockInfo> request() const;

    /**
     * Process response
     */
    void onResponse(const WarpSyncProof &res);

    /**
     * Sync to specified justification.
     * Doesn't check justification chain.
     */
    void unsafe(const BlockHeader &header,
                const GrandpaJustification &j,
                consensus::grandpa::AuthoritySetId set);

   private:
    void applyInner(const Op &op);

    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<consensus::grandpa::JustificationObserver> grandpa_;
    std::shared_ptr<blockchain::BlockStorage> block_storage_;
    std::shared_ptr<network::WarpSyncCache> warp_sync_cache_;
    std::shared_ptr<consensus::grandpa::AuthorityManager> authority_manager_;
    std::shared_ptr<consensus::grandpa::IVerifiedJustificationQueue>
        verified_justification_queue_;
    std::shared_ptr<consensus::babe::BabeConfigRepository>
        babe_config_repository_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<storage::BufferStorage> db_;
    bool done_ = false;

    log::Logger log_;
  };
}  // namespace kagome::network
