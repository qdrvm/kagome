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

#include "scale/tie.hpp"

#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "log/profiling_logger.hpp"

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
      OUTDATED_PRUNE_BASE
    };

    static inline const common::Buffer TRIE_PRUNER_INFO_KEY =
        ":trie_pruner:info"_buf;

    struct TriePrunerInfo {
      SCALE_TIE(2);

      primitives::BlockInfo prune_base;
      std::vector<std::pair<primitives::BlockHash,
                            std::vector<storage::trie::RootHash>>>
          child_states;
    };

    outcome::result<void> init(const blockchain::BlockTree &block_tree);

    static outcome::result<std::unique_ptr<TriePrunerImpl>> create(
        std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer,
        std::shared_ptr<storage::trie::Codec> codec,
        std::shared_ptr<storage::SpacedStorage> storage,
        std::shared_ptr<crypto::Hasher> hasher);

    virtual outcome::result<void> addNewState(
        trie::PolkadotTrie const &new_trie,
        trie::StateVersion version) override;

    virtual outcome::result<void> addNewChildState(
        storage::trie::RootHash const& parent_root,
        trie::PolkadotTrie const &new_trie,
        trie::StateVersion version) override;

    virtual outcome::result<void> markAsChild(
        Parent parent,
        Child child) override;

    virtual outcome::result<void> pruneFinalized(
        primitives::BlockHeader const &state,
        primitives::BlockInfo const &next_block) override;

    virtual outcome::result<void> pruneDiscarded(
        primitives::BlockHeader const &state) override;

    primitives::BlockNumber getBaseBlock() const {
      return base_block_.number;
    }

    size_t getTrackedNodesNum() const {
      return ref_count_.size();
    }

    std::set<common::Buffer> generateTrackedNodeSet() const {
      std::set<common::Buffer> set;
      for (auto &[node, count] : ref_count_) {
        set.insert(node);
      }
      return set;
    }

    size_t getRefCountOf(common::Buffer const &node) const {
      auto it = ref_count_.find(node);
      return it == ref_count_.end() ? 0 : it->second;
    }

    template <typename F>
    void forRefCounts(F const &f) {
      for (auto &[node, count] : ref_count_) {
        f(node, count);
      }
    }

   private:
    TriePrunerImpl(
        std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer,
        std::shared_ptr<storage::trie::Codec> codec,
        std::shared_ptr<storage::SpacedStorage> storage,
        std::shared_ptr<crypto::Hasher> hasher)
        : trie_storage_{trie_storage},
          serializer_{serializer},
          codec_{codec},
          storage_{storage},
          hasher_{hasher} {
      BOOST_ASSERT(trie_storage_ != nullptr);
      BOOST_ASSERT(serializer_ != nullptr);
      BOOST_ASSERT(codec_ != nullptr);
      BOOST_ASSERT(storage_ != nullptr);
      BOOST_ASSERT(hasher_ != nullptr);
    }

    outcome::result<void> prune(primitives::BlockHeader const &state);

    struct AddConfig {
      enum AddType {
        AddLoadedOnly,
        AddNewLoadDummies,
        AddWholeState,
      } type;

      bool shouldLoadDummies() const {
        switch (type) {
          case AddLoadedOnly:
            return false;
          case AddNewLoadDummies:
            return true;
          case AddWholeState:
            return true;
        }
      }

      bool shouldAddAllNodes() const {
        switch (type) {
          case AddLoadedOnly:
            return false;
          case AddNewLoadDummies:
            return false;
          case AddWholeState:
            return true;
        }
      }
    };

    outcome::result<storage::trie::RootHash> addNewStateWith(
        trie::PolkadotTrie const &new_trie,
        trie::StateVersion version,
        AddConfig config);

    outcome::result<void> restoreState(
        primitives::BlockHeader const &base_block,
        blockchain::BlockTree const &block_tree);

    outcome::result<void> storeInfo() const;

    std::unordered_map<common::Buffer, size_t> ref_count_;

    primitives::BlockInfo base_block_{};
    std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<storage::trie::Codec> codec_;
    std::shared_ptr<storage::SpacedStorage> storage_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::unordered_map<storage::trie::RootHash,
                       std::vector<storage::trie::RootHash>>
        child_states_;
    log::Logger logger_ = log::createLogger("TriePruner", "trie_pruner");
  };

}  // namespace kagome::storage::trie_pruner

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie_pruner, TriePrunerImpl::Error);

#endif  // KAGOME_TRIE_PRUNER_IMPL_HPP
