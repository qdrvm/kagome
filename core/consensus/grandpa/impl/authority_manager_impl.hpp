/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL
#define KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL

#include "consensus/grandpa/authority_manager.hpp"
#include "consensus/grandpa/grandpa_digest_observer.hpp"

#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "primitives/authority.hpp"
#include "primitives/block_header.hpp"
#include "primitives/event_types.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::application {
  class AppStateManager;
}
namespace kagome::consensus::grandpa {
  class ScheduleNode;
}
namespace kagome::blockchain {
  class BlockTree;
  class BlockHeaderRepository;
}  // namespace kagome::blockchain

namespace kagome::primitives {
  struct BabeConfiguration;
}  // namespace kagome::primitives

namespace kagome::runtime {
  class GrandpaApi;
  class Executor;
}  // namespace kagome::runtime

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::consensus::grandpa {

  class AuthorityManagerImpl final
      : public AuthorityManager,
        public GrandpaDigestObserver,
        public std::enable_shared_from_this<AuthorityManagerImpl> {
   public:
    inline static const std::vector<primitives::ConsensusEngineId>
        kKnownEngines{primitives::kBabeEngineId, primitives::kGrandpaEngineId};

    static const primitives::BlockNumber kSavepointEachSuchBlock = 100000;

    struct Config {
      // Whether OnDisabled digest message should be processed.
      // It is disabled in Polkadot.
      // It is enabled in Kusama, but some blocks (recognized in 530k-550k)
      // fail to finalize and syncing gets stuck
      bool on_disable_enabled = false;
    };

    AuthorityManagerImpl(
        Config config,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<storage::trie::TrieStorage> trie_storage,
        std::shared_ptr<runtime::GrandpaApi> grandpa_api,
        std::shared_ptr<crypto::Hasher> hash,
        std::shared_ptr<storage::BufferStorage> persistent_storage,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
        primitives::events::ChainSubscriptionEnginePtr chain_events_engine);

    ~AuthorityManagerImpl() override;

    bool prepare();

    // GrandpaDigestObserver

    outcome::result<void> onDigest(
        const primitives::BlockInfo &block,
        const consensus::BabeBlockHeader &digest) override;

    outcome::result<void> onDigest(
        const primitives::BlockInfo &block,
        const primitives::GrandpaDigest &digest) override;

    void cancel(const primitives::BlockInfo &block) override;

    // AuthorityManager

    primitives::BlockInfo base() const override;

    std::optional<std::shared_ptr<const primitives::AuthoritySet>> authorities(
        const primitives::BlockInfo &target_block,
        IsBlockFinalized finalized) const override;

    outcome::result<void> applyScheduledChange(
        const primitives::BlockInfo &block,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activate_at) override;

    outcome::result<void> applyForcedChange(
        const primitives::BlockInfo &block,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber delay_start,
        size_t delay) override;

    outcome::result<void> applyOnDisabled(const primitives::BlockInfo &block,
                                          uint64_t authority_index) override;

    outcome::result<void> applyPause(
        const primitives::BlockInfo &block,
        primitives::BlockNumber activate_at) override;

    outcome::result<void> applyResume(
        const primitives::BlockInfo &block,
        primitives::BlockNumber activate_at) override;

   private:
    void prune(const primitives::BlockInfo &block);

    outcome::result<void> load();
    outcome::result<void> save();

    /**
     * @brief Find schedule_node according to the block
     * @param block for which to find the schedule node
     * @return oldest schedule_node according to the block
     */
    std::shared_ptr<ScheduleNode> getAppropriateAncestor(
        const primitives::BlockInfo &block) const;

    /**
     * @brief Find node according to the block
     * @param block for which to find the schedule node
     * @return oldest node according to the block
     */
    std::shared_ptr<ScheduleNode> getNode(
        const primitives::BlockInfo &block) const {
      return getAppropriateAncestor(block);
    }

    /**
     * @brief Check if one block is direct ancestor of second one
     * @param ancestor - hash of block, which is at the top of the chain
     * @param descendant - hash of block, which is the bottom of the chain
     * @return true if \param ancestor is direct ancestor of \param descendant
     */
    bool directChainExists(const primitives::BlockInfo &ancestor,
                           const primitives::BlockInfo &descendant) const;

    void reorganize(std::shared_ptr<ScheduleNode> node,
                    std::shared_ptr<ScheduleNode> new_node);

    Config config_;
    std::shared_ptr<const blockchain::BlockTree> block_tree_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;
    std::shared_ptr<runtime::GrandpaApi> grandpa_api_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<storage::BufferStorage> persistent_storage_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;

    std::shared_ptr<ScheduleNode> root_;
    primitives::BlockNumber last_saved_state_block_ = 0;

    log::Logger logger_;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL
