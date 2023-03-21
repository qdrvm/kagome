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

#include "blockchain/block_storage.hpp"
#include "common/buffer.hpp"
#include "log/logger.hpp"
#include "log/profiling_logger.hpp"
#include "storage/spaced_storage.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {
  class TrieStorageBackend;
  class TrieSerializer;
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
        ":trie_pruner_info"_buf;

    struct TriePrunerInfo {
      SCALE_TIE(1);

      primitives::BlockNumber prune_base;
    };

    outcome::result<void> init(
        std::shared_ptr<blockchain::BlockStorage> block_storage);

    TriePrunerImpl(
        std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage,
        std::shared_ptr<storage::trie::TrieSerializer> serializer,
        std::shared_ptr<storage::trie::Codec> codec,
        std::shared_ptr<storage::SpacedStorage> storage)
        : trie_storage_{trie_storage},
          serializer_{serializer},
          codec_{codec},
          storage_{storage} {
      BOOST_ASSERT(trie_storage_ != nullptr);
      BOOST_ASSERT(serializer_ != nullptr);
      BOOST_ASSERT(codec_ != nullptr);
      BOOST_ASSERT(storage_ != nullptr);
    }

    virtual outcome::result<void> addNewState(
        trie::PolkadotTrie const &new_trie,
        trie::StateVersion version) override;

    virtual outcome::result<void> prune(primitives::BlockHeader const &state,
                                        trie::StateVersion version) override;

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
    outcome::result<void> addNewStateCompletely(
        trie::PolkadotTrie const &new_trie, trie::StateVersion version);

    outcome::result<void> pruneCompletely(primitives::BlockHeader const &state,
                                          trie::StateVersion version);

    outcome::result<void> printTree(const trie::PolkadotTrie &trie,
                                    trie::StateVersion version) const;

    std::unordered_map<common::Buffer, size_t> ref_count_;

    std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<storage::trie::Codec> codec_;
    std::shared_ptr<storage::SpacedStorage> storage_;
    log::Logger logger_ = log::createLogger("TriePruner", "trie_pruner");
  };

}  // namespace kagome::storage::trie_pruner

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie_pruner, TriePrunerImpl::Error);

#endif  // KAGOME_TRIE_PRUNER_IMPL_HPP
