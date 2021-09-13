/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_RUNTIME_UPGRADE_TRACKER_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_RUNTIME_UPGRADE_TRACKER_HPP

#include "runtime/runtime_upgrade_tracker.hpp"

#include <memory>

#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/event_types.hpp"
#include "storage/trie/types.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::runtime {

  class RuntimeUpgradeTrackerImpl final : public RuntimeUpgradeTracker {
   public:
    RuntimeUpgradeTrackerImpl(
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo);

    void subscribeToBlockchainEvents(
        std::shared_ptr<primitives::events::ChainSubscriptionEngine>
            chain_sub_engine,
        std::shared_ptr<const blockchain::BlockTree> block_tree);

    outcome::result<storage::trie::RootHash> getLastCodeUpdateState(
        const primitives::BlockInfo &block) const;

   private:
    bool isStateInChain(const primitives::BlockInfo &state,
                        const primitives::BlockInfo &chain_end) const;

    struct RuntimeUpgradeData {

      template <typename BlockInfo, typename RootHash>
      RuntimeUpgradeData(BlockInfo &&block, RootHash &&state)
          : block{std::forward<BlockInfo>(block)},
            state{std::forward<RootHash>(state)} {}

      primitives::BlockInfo block;
      storage::trie::RootHash state;
    };

    outcome::result<boost::optional<storage::trie::RootHash>> findProperFork(
        const primitives::BlockInfo &block,
        std::vector<RuntimeUpgradeData>::const_iterator latest_state_update_it)
        const;

    // assumption: insertions in the middle should be extremely rare, if any
    // assumption: runtime upgrades are rare
    mutable std::vector<RuntimeUpgradeData> runtime_upgrades_;

    std::shared_ptr<primitives::events::ChainEventSubscriber>
        chain_subscription_;
    std::shared_ptr<const blockchain::BlockTree> block_tree_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_RUNTIME_UPGRADE_TRACKER_HPP
