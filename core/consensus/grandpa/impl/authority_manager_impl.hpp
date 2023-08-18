/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL
#define KAGOME_CONSENSUS_AUTHORITIES_MANAGER_IMPL

#include "consensus/grandpa/authority_manager.hpp"
#include "consensus/grandpa/grandpa_digest_observer.hpp"

#include <mutex>

#include "log/logger.hpp"
#include "primitives/authority.hpp"
#include "primitives/block_header.hpp"
#include "primitives/event_types.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::application {
  class AppStateManager;
}

namespace kagome::consensus::grandpa {
  class ScheduleNode;
}

namespace kagome::crypto {
  class Hasher;
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

namespace kagome::consensus::grandpa {

  class AuthorityManagerImpl final
      : public AuthorityManager,
        public GrandpaDigestObserver,
        public std::enable_shared_from_this<AuthorityManagerImpl> {
   public:
    inline static const std::vector<primitives::ConsensusEngineId>
        kKnownEngines{primitives::kBabeEngineId, primitives::kGrandpaEngineId};

    static const primitives::BlockNumber kSavepointBlockInterval = 100000;

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
        std::shared_ptr<runtime::GrandpaApi> grandpa_api,
        std::shared_ptr<crypto::Hasher> hash,
        std::shared_ptr<storage::SpacedStorage> persistent_storage,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
        primitives::events::ChainSubscriptionEnginePtr chain_events_engine);

    ~AuthorityManagerImpl() override;

    bool prepare();

    // GrandpaDigestObserver

    outcome::result<void> onDigest(
        const primitives::BlockContext &context,
        const consensus::babe::BabeBlockHeader &digest) override;

    outcome::result<void> onDigest(
        const primitives::BlockContext &context,
        const primitives::GrandpaDigest &digest) override;

    void cancel(const primitives::BlockInfo &block) override;

    // AuthorityManager

    std::optional<std::shared_ptr<const primitives::AuthoritySet>> authorities(
        const primitives::BlockInfo &target_block,
        IsBlockFinalized finalized) const override;

    void warp(const primitives::BlockInfo &block,
              const primitives::BlockHeader &header,
              const primitives::AuthoritySet &authorities) override;

   private:
    // TODO(Harrm): Refactor me to something safer than hoping
    // that nobody would forget to call the proper method
    outcome::result<void> onDigestNoLock(
        const primitives::BlockContext &context,
        const consensus::babe::BabeBlockHeader &digest);

    outcome::result<void> onDigestNoLock(
        const primitives::BlockContext &context,
        const primitives::GrandpaDigest &digest);

    /**
     * @brief Schedule an authority set change after the given delay of N
     * blocks, after next one would be finalized by the finality consensus
     * engine
     * @param context data about block where a digest with this change was
     * discovered
     * @param authorities is authority set for renewal
     * @param activateAt is number of block after finalization of which changes
     * will be applied
     */
    outcome::result<void> applyScheduledChange(
        const primitives::BlockContext &context,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activate_at);

    /**
     * @brief Force an authority set change after the given delay of N blocks,
     * after next one would be imported block which has been validated by the
     * block production consensus engine.
     * @param context data about block where a digest with this change was
     * discovered
     * @param authorities new authority set
     * @param activateAt is number of block when changes will applied
     */
    outcome::result<void> applyForcedChange(
        const primitives::BlockContext &context,
        const primitives::AuthorityList &authorities,
        primitives::BlockNumber activate_at);

    /**
     * @brief An index of the individual authority in the current authority list
     * that should be immediately disabled until the next authority set change.
     * When an authority gets disabled, the node should stop performing any
     * authority functionality from that authority, including authoring blocks
     * and casting GRANDPA votes for finalization. Similarly, other nodes should
     * ignore all messages from the indicated authority which pertain to their
     * authority role. Once an authority set change after the given delay of N
     * blocks, is an imported block which has been validated by the block
     * production consensus engine.
     * @param context data about block where a digest with this change was
     * discovered
     * @param authority_index is index of one authority in current authority set
     */

    outcome::result<void> applyOnDisabled(
        const primitives::BlockContext &context,
        primitives::AuthorityIndex authority_index);

    /**
     * @brief A signal to pause the current authority set after the given delay,
     * is a block finalized by the finality consensus engine. After finalizing
     * block, the authorities should stop voting.
     * @param context data about block where a digest with this change was
     * discovered
     * @param activateAt is number of block after finalization of which changes
     * will be applied
     */
    outcome::result<void> applyPause(const primitives::BlockContext &context,
                                     primitives::BlockNumber activate_at);

    /**
     * @brief A signal to resume the current authority set after the given
     * delay, is an imported block and validated by the block production
     * consensus engine. After authoring the block B 0 , the authorities should
     * resume voting.
     * @param context data about block where a digest with this change was
     * discovered
     * @param activateAt is number of block when changes will applied
     */
    outcome::result<void> applyResume(const primitives::BlockContext &context,
                                      primitives::BlockNumber activate_at);

    void prune(const primitives::BlockInfo &block);

    outcome::result<void> load();
    outcome::result<void> save();

    /**
     * @brief Find node according to the block
     * @param block for which to find the schedule node
     * @return oldest node according to the block
     */
    std::shared_ptr<ScheduleNode> getNode(
        const primitives::BlockContext &context) const;

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

    mutable std::mutex mutex_;

    Config config_;
    std::shared_ptr<const blockchain::BlockTree> block_tree_;
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
