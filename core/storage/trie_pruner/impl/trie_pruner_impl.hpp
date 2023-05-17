/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_PRUNER_IMPL_HPP
#define KAGOME_TRIE_PRUNER_IMPL_HPP

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

namespace kagome::application {
  class AppConfiguration;
}

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::blockchain {
  class BlockTree;
  class BlockStorage;
}  // namespace kagome::blockchain

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
      CREATE_PRUNER_ON_NON_PRUNED_NON_EMPTY_STORAGE = 1,
      OUTDATED_PRUNE_BASE,
      LAST_PRUNED_BLOCK_IS_LAST_FINALIZED,
    };

    static inline const common::Buffer TRIE_PRUNER_INFO_KEY =
        ":trie_pruner:info"_buf;

    struct ChildStorageInfo {
      SCALE_TIE(2);

      common::Buffer key;
      storage::trie::RootHash root;
    };

    struct TriePrunerInfo {
      SCALE_TIE(2);

      std::optional<primitives::BlockInfo> last_pruned_block;
      std::vector<
          std::pair<primitives::BlockHash, std::vector<ChildStorageInfo>>>
          child_states;
    };

    outcome::result<void> init(const blockchain::BlockTree &block_tree);

    static outcome::result<std::unique_ptr<TriePrunerImpl>> create(
        std::shared_ptr<const application::AppConfiguration> config,
        std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage,
        std::shared_ptr<const storage::trie::TrieSerializer> serializer,
        std::shared_ptr<const storage::trie::Codec> codec,
        std::shared_ptr<storage::SpacedStorage> storage,
        std::shared_ptr<const crypto::Hasher> hasher);

    virtual outcome::result<void> addNewState(
        trie::PolkadotTrie const &new_trie,
        trie::StateVersion version) override;

    virtual outcome::result<void> addNewChildState(
        storage::trie::RootHash const &parent_root,
        common::BufferView key,
        trie::PolkadotTrie const &new_trie,
        trie::StateVersion version) override;

    virtual outcome::result<void> markAsChild(Parent parent,
                                              common::Buffer const &key,
                                              Child child) override;

    virtual outcome::result<void> pruneFinalized(
        primitives::BlockHeader const &state) override;

    virtual outcome::result<void> pruneDiscarded(
        primitives::BlockHeader const &state) override;

    std::optional<primitives::BlockNumber> getLastPrunedBlock() const override {
      if (last_pruned_block_) {
        return last_pruned_block_.value().number;
      }
      return std::nullopt;
    }

    size_t getTrackedNodesNum() const {
      return ref_count_.size();
    }

    size_t getRefCountOf(common::Hash256 const &node) const {
      auto it = ref_count_.find(node);
      return it == ref_count_.end() ? 0 : it->second;
    }

    template <typename F>
    void forRefCounts(F const &f) {
      for (auto &[node, count] : ref_count_) {
        f(node, count);
      }
    }

    std::optional<uint32_t> getPruningDepth() const override {
      return pruning_depth_;
    }

   private:
    TriePrunerImpl(
        std::optional<uint32_t> pruning_depth,
        std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage,
        std::shared_ptr<const storage::trie::TrieSerializer> serializer,
        std::shared_ptr<const storage::trie::Codec> codec,
        std::shared_ptr<storage::SpacedStorage> storage,
        std::shared_ptr<const crypto::Hasher> hasher)
        : trie_storage_{trie_storage},
          serializer_{serializer},
          codec_{codec},
          storage_{storage},
          hasher_{hasher},
          pruning_depth_{pruning_depth} {
      BOOST_ASSERT(trie_storage_ != nullptr);
      BOOST_ASSERT(serializer_ != nullptr);
      BOOST_ASSERT(codec_ != nullptr);
      BOOST_ASSERT(storage_ != nullptr);
      BOOST_ASSERT(hasher_ != nullptr);
    }

    outcome::result<void> prune(primitives::BlockHeader const &state);

    outcome::result<storage::trie::RootHash> addNewStateWith(
        trie::PolkadotTrie const &new_trie,
        trie::StateVersion version);

    // resets the pruner state and builds it from scratch from the state
    // of the provided block
    outcome::result<void> restoreState(
        primitives::BlockHeader const &base_block,
        blockchain::BlockTree const &block_tree);

    // store the persistent pruner info to the database
    outcome::result<void> savePersistentState() const;

    std::mutex ref_count_mutex_;
    std::unordered_map<common::Buffer, size_t> ref_count_;
    std::unordered_map<common::Hash256, size_t> value_ref_count_;

    std::optional<primitives::BlockInfo> last_pruned_block_;
    std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage_;
    std::shared_ptr<const storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<const storage::trie::Codec> codec_;
    std::shared_ptr<storage::SpacedStorage> storage_;
    std::shared_ptr<const crypto::Hasher> hasher_;

    std::unordered_map<storage::trie::RootHash, std::vector<ChildStorageInfo>>
        child_states_;
    const std::optional<uint32_t> pruning_depth_{};
    log::Logger logger_ = log::createLogger("TriePruner", "trie_pruner");
  };

}  // namespace kagome::storage::trie_pruner

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie_pruner, TriePrunerImpl::Error);

#endif  // KAGOME_TRIE_PRUNER_IMPL_HPP
