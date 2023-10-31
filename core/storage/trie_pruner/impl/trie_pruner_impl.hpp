/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie_pruner/trie_pruner.hpp"

#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/assert.hpp>

#include "scale/tie.hpp"

#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "log/profiling_logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::application {
  class AppConfiguration;
  class AppStateManager;
}  // namespace kagome::application

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::storage {
  class SpacedStorage;
}

namespace kagome::storage::trie {
  class TrieStorageBackend;
  class TrieSerializer;
  class Codec;
}  // namespace kagome::storage::trie

namespace kagome::storage::trie_pruner {

  using common::literals::operator""_buf;

  class TriePrunerImpl final : public TriePruner {
   public:
    enum class Error {
      LAST_PRUNED_BLOCK_IS_LAST_FINALIZED = 1,
    };

    inline static const common::Buffer TRIE_PRUNER_INFO_KEY =
        ":trie_pruner:info"_buf;

    struct TriePrunerInfo {
      SCALE_TIE(1);

      std::optional<primitives::BlockInfo> last_pruned_block;
    };

    TriePrunerImpl(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage,
        std::shared_ptr<const storage::trie::TrieSerializer> serializer,
        std::shared_ptr<const storage::trie::Codec> codec,
        std::shared_ptr<storage::SpacedStorage> storage,
        std::shared_ptr<const crypto::Hasher> hasher,
        std::shared_ptr<const application::AppConfiguration> config);

    bool prepare();

    virtual outcome::result<void> addNewState(
        const storage::trie::RootHash &state_root,
        trie::StateVersion version) override;

    virtual outcome::result<void> addNewState(
        const trie::PolkadotTrie &new_trie,
        trie::StateVersion version) override;

    virtual outcome::result<void> pruneFinalized(
        const primitives::BlockHeader &state) override;

    virtual outcome::result<void> pruneDiscarded(
        const primitives::BlockHeader &state) override;

    std::optional<primitives::BlockInfo> getLastPrunedBlock() const override {
      return last_pruned_block_;
    }

    size_t getTrackedNodesNum() const {
      return ref_count_.size();
    }

    size_t getRefCountOf(const common::Hash256 &node) const {
      auto it = ref_count_.find(node);
      return it == ref_count_.end() ? 0 : it->second;
    }

    template <typename F>
    void forRefCounts(const F &f) {
      for (auto &[node, count] : ref_count_) {
        f(node, count);
      }
    }

    std::optional<uint32_t> getPruningDepth() const override {
      return pruning_depth_;
    }

    outcome::result<void> recoverState(
        const blockchain::BlockTree &block_tree) override;

    void reload(const blockchain::BlockTree &block_tree) override;

   private:
    outcome::result<void> restoreStateAt(
        const primitives::BlockHeader &last_pruned_block,
        const blockchain::BlockTree &block_tree);

    outcome::result<void> prune(BufferBatch &batch,
                                const storage::trie::RootHash &state);

    outcome::result<storage::trie::RootHash> addNewStateWith(
        const trie::PolkadotTrie &new_trie, trie::StateVersion version);

    // store the persistent pruner info to the database
    outcome::result<void> savePersistentState() const;

    std::mutex ref_count_mutex_;
    std::unordered_map<common::Hash256, size_t> ref_count_;
    std::unordered_map<common::Hash256, size_t> value_ref_count_;
    std::unordered_set<common::Hash256> immortal_nodes_;

    std::optional<primitives::BlockInfo> last_pruned_block_;
    std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage_;
    std::shared_ptr<const storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<const storage::trie::Codec> codec_;
    std::shared_ptr<storage::SpacedStorage> storage_;
    std::shared_ptr<const crypto::Hasher> hasher_;

    const std::optional<uint32_t> pruning_depth_{};
    const bool thorough_pruning_{false};
    log::Logger logger_ = log::createLogger("TriePruner", "trie_pruner");
  };

}  // namespace kagome::storage::trie_pruner

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie_pruner, TriePrunerImpl::Error);
