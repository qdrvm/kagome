/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP
#define KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP

#include <set>

#include "blockchain/block_header_repository.hpp"
#include "blockchain/block_tree.hpp"
#include "log/logger.hpp"
#include "primitives/block_id.hpp"
#include "primitives/event_types.hpp"
#include "runtime/runtime_code_provider.hpp"

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::runtime {

  inline const common::Buffer kRuntimeCodeKey = common::Buffer().put(":code");

  class StorageCodeProvider final : public RuntimeCodeProvider {
   public:
    ~StorageCodeProvider() override = default;

    explicit StorageCodeProvider(
        std::shared_ptr<const storage::trie::TrieStorage> storage);

    void subscribeToBlockchainEvents(
        std::shared_ptr<primitives::events::StorageSubscriptionEngine>
            storage_sub_engine,
        std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<const blockchain::BlockTree> block_tree);

    outcome::result<CodeAndItsState> getCodeAt(
        const primitives::BlockInfo &block) const override;

    outcome::result<CodeAndItsState> getLatestCode() const override;

   private:
    outcome::result<boost::optional<storage::trie::RootHash>>
    getLastCodeUpdateState(const primitives::BlockInfo &block) const;

    // assumption: insertions in the middle should be extremely rare, if any
    // assumption: runtime upgrades are rare
    std::vector<primitives::BlockInfo> blocks_with_runtime_upgrade_;
    std::shared_ptr<primitives::events::StorageEventSubscriber>
        storage_subscription_;
    std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<const blockchain::BlockTree> block_tree_;
    std::shared_ptr<const storage::trie::TrieStorage> storage_;
    mutable common::Buffer state_code_;
    mutable storage::trie::RootHash last_state_root_;
    log::Logger logger_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_STORAGE_WASM_PROVIDER_HPP
