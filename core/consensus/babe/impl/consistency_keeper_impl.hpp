/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERIMPL
#define KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERIMPL

#include "consensus/babe/consistency_keeper.hpp"

#include <memory>

#include "log/logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::application {
  class AppStateManager;
};

namespace kagome::blockchain {
  class DigestTracker;
  class BlockTree;
};  // namespace kagome::blockchain

namespace kagome::consensus::babe {

  class ConsistencyKeeperImpl final : public ConsistencyKeeper {
   public:
    ConsistencyKeeperImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<storage::BufferStorage> storage,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<blockchain::DigestTracker> digest_tracker);

    ~ConsistencyKeeperImpl() override = default;

    Guard start(primitives::BlockInfo block) override;

   protected:
    void commit(primitives::BlockInfo block) override;
    void rollback(primitives::BlockInfo block) override;

   private:
    bool prepare();
    void cleanup();

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::DigestTracker> digest_tracker_;

    log::Logger logger_;
    std::atomic_bool in_progress_{false};
  };

}  // namespace kagome::consensus::babe

#endif  //  KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERIMPL
