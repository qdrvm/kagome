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
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {
  class TrieStorageBackend;
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::storage::trie_pruner {

  inline outcome::result<void> forSubtree(
      trie::TrieNode &root,
      const trie::TrieSerializer &serializer,
      std::function<outcome::result<void>(
          trie::TrieNode const *, trie::TrieNode const &)> const &callback) {
    struct QueueEntry {
      trie::TrieNode *node;
      uint8_t next_child_idx = 0;
    };
    auto logger = log::createLogger("Trie Pruner");
    std::vector<QueueEntry> queued_nodes;
    queued_nodes.push_back({&root, 0});
    while (!queued_nodes.empty()) {
      auto &entry = queued_nodes.back();
      auto parent =
          queued_nodes.size() > 1 ? (queued_nodes.end() - 2)->node : nullptr;

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
          OUTCOME_TRY(callback(parent, *entry.node));
          queued_nodes.pop_back();
        }
      } else {
        OUTCOME_TRY(callback(parent, *entry.node));
        queued_nodes.pop_back();
      }
    }
    return outcome::success();
  }

  class TriePrunerImpl final : public TriePruner {
   public:
    TriePrunerImpl(std::shared_ptr<storage::trie::TrieStorageBackend> storage,
                   std::shared_ptr<storage::trie::TrieSerializer> serializer,
                   std::shared_ptr<storage::trie::Codec> codec)
        : storage_{storage}, serializer_{serializer}, codec_{codec} {
      BOOST_ASSERT(storage_ != nullptr);
      BOOST_ASSERT(serializer_ != nullptr);
      BOOST_ASSERT(codec_ != nullptr);
    }

    virtual outcome::result<void> addNewState(
        trie::PolkadotTrie const &new_trie) override {
      logger_->info("ref count map size is {}", ref_count_.size());
      KAGOME_PROFILE_START_L(logger_, register_state);
      std::vector<const trie::OpaqueTrieNode *> queued_nodes;
      queued_nodes.push_back(new_trie.getRoot().get());

      size_t referenced_nodes_num = 0;

      while (!queued_nodes.empty()) {
        auto opaque_node = queued_nodes.back();
        queued_nodes.pop_back();
        Buffer hash;
        if (auto hash_opt = opaque_node->getCachedHash();
            hash_opt.has_value()) {
          hash = hash_opt.value();
        } else {
          // we encode nodes twice: here and during serialization to DB. Should
          // avoid it somehow
          // mind that encode node's callback doesn't report dummy nodes
          OUTCOME_TRY(enc,
                      codec_->encodeNode(
                          *opaque_node,
                          trie::StateVersion::V1,
                          [](auto &, auto, auto &&) -> outcome::result<void> {
                            return outcome::success();
                          }));
          hash = codec_->merkleValue(enc);
        }
        ref_count_[hash]++;
        referenced_nodes_num++;
        if (auto node = dynamic_cast<trie::TrieNode const *>(opaque_node);
            node != nullptr && node->isBranch()) {
          auto branch = dynamic_cast<const trie::BranchNode *>(opaque_node);
          for (auto opaque_child : branch->children) {
            if (opaque_child != nullptr) {
              queued_nodes.push_back(opaque_child.get());
            }
          }
        }
      }
      SL_INFO(logger_, "Referenced {} nodes", referenced_nodes_num);
      return outcome::success();
    }

    virtual outcome::result<void> prune(trie::RootHash const &state) override {
      OUTCOME_TRY(trie, serializer_->retrieveTrie(state));
      size_t removed = 0;
      size_t unknown = 0;

      std::vector<std::shared_ptr<trie::TrieNode>> queued_nodes;
      queued_nodes.push_back(trie->getRoot());

      // iterate all nodes with ref count now 0 and delete them from DB
      while (!queued_nodes.empty()) {
        auto node = queued_nodes.back();
        queued_nodes.pop_back();
        auto ref_count_it = ref_count_.find(node->getCachedHash().value());
        if (ref_count_it == ref_count_.end()) {
          unknown++;
          continue;
        }
        auto& ref_count = ref_count_it->second;
        BOOST_ASSERT(ref_count != 0);
        ref_count--;
        if (ref_count == 0) {
          removed++;
          ref_count_.erase(node->getCachedHash().value());
          OUTCOME_TRY(storage_->remove(node->getCachedHash().value()));
          if (node->isBranch()) {
            auto branch = static_cast<const trie::BranchNode &>(*node);
            for (auto opaque_child : branch.children) {
              if (opaque_child != nullptr) {
                OUTCOME_TRY(child, serializer_->retrieveNode(opaque_child));
                queued_nodes.push_back(child);
              }
            }
          }
        }
      }

      SL_INFO(logger_, "Removed {} nodes, {} unknown", removed, unknown);
      return outcome::success();
    }

    size_t getTrackedNodesNum() const {
      return ref_count_.size();
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
    std::unordered_map<common::Buffer, size_t> ref_count_;

    std::shared_ptr<storage::trie::TrieStorageBackend> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<storage::trie::Codec> codec_;
    log::Logger logger_ = log::createLogger("TriePruner", "trie");
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIE_PRUNER_IMPL_HPP
