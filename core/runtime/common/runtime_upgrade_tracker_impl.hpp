/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_RUNTIME_UPGRADE_TRACKER_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_RUNTIME_UPGRADE_TRACKER_HPP

#include "runtime/runtime_upgrade_tracker.hpp"

#include <memory>

#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/event_types.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/trie/types.hpp"

namespace kagome::blockchain {
  class BlockHeaderRepository;
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::runtime {

  class RuntimeUpgradeTrackerImpl final : public RuntimeUpgradeTracker {
   public:
    RuntimeUpgradeTrackerImpl(
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<storage::BufferStorage> storage,
        const application::CodeSubstitutes &code_substitutes);

    void subscribeToBlockchainEvents(
        std::shared_ptr<primitives::events::StorageSubscriptionEngine>
            storage_sub_engine,
        std::shared_ptr<const blockchain::BlockTree> block_tree);

    outcome::result<primitives::BlockId> getRuntimeChangeBlock(
        const primitives::BlockInfo &block);

   private:
    bool hasCodeSubstitute(const kagome::primitives::BlockHash &hash) const;
    void save();
    // assumption: insertions in the middle should be extremely rare, if any
    // assumption: runtime upgrades are rare
    std::vector<primitives::BlockInfo> runtime_upgrade_parents_;

    std::shared_ptr<primitives::events::StorageEventSubscriber>
        storage_subscription_;
    std::shared_ptr<const blockchain::BlockTree> block_tree_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<storage::BufferStorage> storage_;
    const application::CodeSubstitutes &code_substitutes_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_RUNTIME_UPGRADE_TRACKER_HPP
