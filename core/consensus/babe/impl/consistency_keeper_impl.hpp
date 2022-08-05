/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERIMPL
#define KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERIMPL

#include "consensus/babe/consistency_keeper.hpp"

#include <memory>

#include "blockchain/block_tree.hpp"
#include "consensus/authority/authority_update_observer.hpp"
#include "log/logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::application {
  class AppStateManager;
};

namespace kagome::consensus::babe {

  class ConsistencyKeeperImpl final : public ConsistencyKeeper {
   public:
    ConsistencyKeeperImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<storage::BufferStorage> storage,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<authority::AuthorityUpdateObserver>
            authority_update_observer);

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
    std::shared_ptr<authority::AuthorityUpdateObserver>
        authority_update_observer_;

    log::Logger logger_;
    std::atomic_bool in_progress_{false};
  };

}  // namespace kagome::consensus::babe

#endif  //  KAGOME_CONSENSUS_BABE_CONSISTENCYKEEPERIMPL
