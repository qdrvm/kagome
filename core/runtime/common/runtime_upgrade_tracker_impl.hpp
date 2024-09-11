/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_upgrade_tracker.hpp"

#include <memory>

#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/event_types.hpp"
#include "scale/tie.hpp"
#include "storage/spaced_storage.hpp"
#include "storage/trie/types.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
  class BlockTree;
  class BlockStorage;
}  // namespace kagome::blockchain

namespace kagome::runtime {

  class RuntimeUpgradeTrackerImpl final : public RuntimeUpgradeTracker {
   public:
    /**
     * Performs a storage read to fetch saved upgrade states on initialization,
     * which may fail, thus construction only from a factory method
     */
    static outcome::result<std::unique_ptr<RuntimeUpgradeTrackerImpl>> create(
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<storage::SpacedStorage> storage,
        std::shared_ptr<const primitives::CodeSubstituteBlockIds>
            code_substitutes,
        std::shared_ptr<blockchain::BlockStorage> block_storage);

    struct RuntimeUpgradeData {
      SCALE_TIE(2);

      RuntimeUpgradeData() = default;

      template <typename BlockInfo, typename RootHash>
      RuntimeUpgradeData(BlockInfo &&block, RootHash &&state)
          : block{std::forward<BlockInfo>(block)},
            state{std::forward<RootHash>(state)} {}

      primitives::BlockInfo block;
      storage::trie::RootHash state;
    };

    void subscribeToBlockchainEvents(
        std::shared_ptr<primitives::events::ChainSubscriptionEngine>
            chain_sub_engine,
        std::shared_ptr<const blockchain::BlockTree> block_tree);

    outcome::result<storage::trie::RootHash> getLastCodeUpdateState(
        const primitives::BlockInfo &block) override;

    outcome::result<primitives::BlockInfo> getLastCodeUpdateBlockInfo(
        const storage::trie::RootHash &state) const override;

   private:
    RuntimeUpgradeTrackerImpl(
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<storage::SpacedStorage> storage,
        std::shared_ptr<const primitives::CodeSubstituteBlockIds>
            code_substitutes,
        std::vector<RuntimeUpgradeData> &&saved_data,
        std::shared_ptr<blockchain::BlockStorage> block_storage);

    outcome::result<bool> isStateInChain(
        const primitives::BlockInfo &state,
        const primitives::BlockInfo &chain_end) const;

    outcome::result<std::optional<storage::trie::RootHash>> findProperFork(
        const primitives::BlockInfo &block,
        std::vector<RuntimeUpgradeData>::const_reverse_iterator
            latest_upgrade_it) const;

    bool hasCodeSubstitute(
        const kagome::primitives::BlockInfo &block_info) const;

    /**
     * Pushes record [block info + state root] of block whose state contains
     * particular runtime code first time
     * @param hash - hash of such block
     * @return pair of state root hash and true or false if such record is
     * already existing or not, or error
     */
    outcome::result<std::pair<storage::trie::RootHash, bool>> push(
        const primitives::BlockHash &hash);

    void save();

    // assumption: insertions in the middle should be extremely rare, if any
    // assumption: runtime upgrades are rare
    std::vector<RuntimeUpgradeData> runtime_upgrades_;

    std::shared_ptr<primitives::events::ChainEventSubscriber>
        chain_subscription_;
    std::weak_ptr<const blockchain::BlockTree> block_tree_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<const primitives::CodeSubstituteBlockIds>
        known_code_substitutes_;
    std::shared_ptr<blockchain::BlockStorage> block_storage_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime
