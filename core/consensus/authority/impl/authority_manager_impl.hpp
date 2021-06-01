/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL
#define KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL

#include "consensus/authority/authority_manager.hpp"
#include "consensus/authority/authority_update_observer.hpp"

#include "log/logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::application {
  class AppStateManager;
}
namespace kagome::authority {
  class ScheduleNode;
}
namespace kagome::blockchain {
  class BlockTree;
}
namespace kagome::primitives {
  struct AuthorityList;
  struct BabeConfiguration;
}  // namespace kagome::primitives

namespace kagome::authority {
  class AuthorityManagerImpl : public AuthorityManager,
                               public AuthorityUpdateObserver {
   public:
    inline static const std::vector<primitives::ConsensusEngineId>
        known_engines{primitives::kBabeEngineId, primitives::kGrandpaEngineId};
    inline static const common::Buffer SCHEDULER_TREE =
        common::Buffer{}.put(":kagome:authorities:scheduler_tree");

    AuthorityManagerImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<storage::BufferStorage> storage);

    ~AuthorityManagerImpl() override = default;

    /** @see AppStateManager::takeControl */
    bool prepare();

    /** @see AppStateManager::takeControl */
    bool start();

    /** @see AppStateManager::takeControl */
    void stop();

    outcome::result<std::shared_ptr<const primitives::AuthorityList>>
    authorities(const primitives::BlockInfo &block, bool finalized) override;

    outcome::result<void> applyScheduledChange(
        const primitives::BlockInfo &block,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activate_at) override;

    outcome::result<void> applyForcedChange(
        const primitives::BlockInfo &block,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activate_at) override;

    outcome::result<void> applyOnDisabled(const primitives::BlockInfo &block,
                                          uint64_t authority_index) override;

    outcome::result<void> applyPause(
        const primitives::BlockInfo &block,
        primitives::BlockNumber activate_at) override;

    outcome::result<void> applyResume(
        const primitives::BlockInfo &block,
        primitives::BlockNumber activate_at) override;

    outcome::result<void> onConsensus(
        const primitives::ConsensusEngineId &engine_id,
        const primitives::BlockInfo &block,
        const primitives::Consensus &message) override;

    outcome::result<void> prune(const primitives::BlockInfo &block) override;

   private:
    /**
     * @brief Find schedule_node according to the block
     * @param block for whick find schedule node
     * @return oldest schedule_node according to the block
     */
    std::shared_ptr<ScheduleNode> getAppropriateAncestor(
        const primitives::BlockInfo &block);

    /**
     * @brief Check if one block is direct ancestor of second one
     * @param ancestor - hash of block, which is at the top of the chain
     * @param descendant - hash of block, which is the bottom of the chain
     * @return true if \param ancestor is direct ancestor of \param descendant
     */
    bool directChainExists(const primitives::BlockInfo &ancestor,
                           const primitives::BlockInfo &descendant);

    outcome::result<void> save();

    log::Logger log_;
    std::shared_ptr<application::AppStateManager> app_state_manager_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<ScheduleNode> root_;
  };
}  // namespace kagome::authority

#endif  // KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL
