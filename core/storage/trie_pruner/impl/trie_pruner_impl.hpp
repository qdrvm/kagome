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

#include "log/logger.hpp"
#include "log/profiling_logger.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"

namespace kagome::storage::trie {
  class TrieStorageBackend;
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::storage::trie_pruner {

  outcome::result<void> forSubtree(
      trie::TrieNode &root,
      const trie::TrieSerializer &serializer,
      std::function<outcome::result<void>(trie::TrieNode const &)> const
          &callback) {
    struct QueueEntry {
      trie::TrieNode *node;
      uint8_t next_child_idx = 0;
    };
    auto logger = log::createLogger("Trie Pruner");
    std::vector<QueueEntry> queued_nodes;
    queued_nodes.push_back({&root, 0});
    while (!queued_nodes.empty()) {
      auto &entry = queued_nodes.back();
      if (entry.node->isBranch()) {
        auto branch = dynamic_cast<trie::BranchNode *>(entry.node);
        auto next_child_idx = branch->getNextChildIdxFrom(entry.next_child_idx);

        if (next_child_idx < trie::BranchNode::kMaxChildren) {
          auto &opaque_child = branch->children[next_child_idx];
          OUTCOME_TRY(child, serializer.retrieveNode(opaque_child));
          BOOST_ASSERT(child != nullptr);
          branch->children[next_child_idx] = child;
          entry.next_child_idx++;
          queued_nodes.push_back({child.get(), 0});

        } else {
          OUTCOME_TRY(callback(*entry.node));
          queued_nodes.pop_back();
        }
      } else {
        OUTCOME_TRY(callback(*entry.node));
        queued_nodes.pop_back();
      }
    }
    return outcome::success();
  }

  class TriePrunerImpl final : public TriePruner {
   public:
    TriePrunerImpl(std::shared_ptr<storage::trie::TrieStorageBackend> storage,
                   std::shared_ptr<storage::trie::TrieSerializer> serializer,
                   std::shared_ptr<storage::trie::PolkadotCodec> codec)
        : storage_{storage}, serializer_{serializer}, codec_{codec} {
      BOOST_ASSERT(storage_ != nullptr);
      BOOST_ASSERT(serializer_ != nullptr);
      BOOST_ASSERT(codec_ != nullptr);
    }

    virtual outcome::result<void> addNewState(
        trie::RootHash const &state) override {
      logger_->info("ref count map size is {}", ref_count_.size());
      OUTCOME_TRY(trie, serializer_->retrieveTrie(state));
      KAGOME_PROFILE_START_L(logger_, register_state);
      return forSubtree(*trie->getRoot(),
                 *serializer_,
                 [this](auto &node) -> outcome::result<void> {
                   auto hash = node.getCachedHash();
                   BOOST_ASSERT(hash.has_value());
                   ref_count_[hash.value()]++;
                   return outcome::success();
                 });
    }

    virtual outcome::result<void> prune(trie::RootHash const &state) override {
      OUTCOME_TRY(trie, serializer_->retrieveTrie(state));
      size_t removed = 0;
      OUTCOME_TRY(forSubtree(
          *trie->getRoot(),
          *serializer_,
          [this, &removed](auto &node) -> outcome::result<void> {
            auto hash = node.getCachedHash();
            BOOST_ASSERT(hash.has_value());
            auto& ref_count = ref_count_[hash.value()];
            ref_count--;
            if (ref_count == 0) {
              removed++;
            }
            return outcome::success();
          }));
      SL_INFO(logger_, "Removed {} nodes", removed);
      return outcome::success();
    }

   private:
    static common::Hash256 hashFromBuf(common::Buffer const &buf) {
      BOOST_ASSERT(buf.size() <= common::Hash256::size());
      common::Hash256 hash{};
      std::copy_n(buf.begin(),
                  std::min(buf.size(), common::Hash256::size()),
                  hash.begin());
      return hash;
    }

    std::unordered_map<common::Buffer, size_t> ref_count_;
    std::shared_ptr<storage::trie::TrieStorageBackend> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<storage::trie::PolkadotCodec> codec_;
    log::Logger logger_ = log::createLogger("TriePruner", "trie");
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIE_PRUNER_IMPL_HPP
