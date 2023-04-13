/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_WARP_SYNC_HPP
#define KAGOME_NETWORK_WARP_SYNC_HPP

#include "network/warp/types.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::application {
  class AppStateManager;
}  // namespace kagome::application

namespace kagome::blockchain {
  class BlockTree;
  class BlockStorage;
}  // namespace kagome::blockchain

namespace kagome::consensus::grandpa {
  struct JustificationObserver;
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
  class WarpSync {
   public:
    struct Op {
      SCALE_TIE(4);

      primitives::BlockInfo block_info;
      primitives::BlockHeader header;
      consensus::grandpa::GrandpaJustification justification;
      primitives::AuthoritySet authorities;
    };

    WarpSync(
        application::AppStateManager &app_state_manager,
        std::shared_ptr<crypto::Hasher> hasher,
        storage::SpacedStorage &db,
        std::shared_ptr<consensus::grandpa::JustificationObserver> grandpa,
        std::shared_ptr<blockchain::BlockStorage> block_storage,
        std::shared_ptr<network::WarpSyncCache> warp_sync_cache,
        std::shared_ptr<consensus::grandpa::AuthorityManager> authority_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree);

    void start();

    std::optional<primitives::BlockInfo> request() const;

    void onResponse(const WarpSyncProof &res);

   private:
    void applyInner(const Op &op);

    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<consensus::grandpa::JustificationObserver> grandpa_;
    std::shared_ptr<blockchain::BlockStorage> block_storage_;
    std::shared_ptr<network::WarpSyncCache> warp_sync_cache_;
    std::shared_ptr<consensus::grandpa::AuthorityManager> authority_manager_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<storage::BufferStorage> db_;
    bool done_ = false;
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_WARP_SYNC_HPP
