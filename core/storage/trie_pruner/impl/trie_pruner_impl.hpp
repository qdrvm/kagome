/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_PRUNER_IMPL_HPP
#define KAGOME_TRIE_PRUNER_IMPL_HPP

#include "storage/trie_pruner/trie_pruner.hpp"

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"

namespace kagome::storage::trie {
  class TrieStorageBackend;
  class TrieSerializer;
}  // namespace kagome::storage::trie

namespace kagome::storage::trie_pruner {

  class StateWindow {
   public:
    outcome::result<void> addUpdate(trie::PolkadotTrie const &old_tree,
                                    TrieStateUpdate update) {
      if (!updates_.empty()) {
        BOOST_ASSERT(update.finalized_block
                     == updates_.back().finalized_block + 1);
      }
      std::unordered_set<common::Buffer> removed_nodes;
      for (auto &removed_key : update.removed_keys) {
        trie::OpaqueTrieNode *leaf{};
        OUTCOME_TRY(old_tree.forNodeInPath(
            old_tree.getRoot(),
            trie::KeyNibbles::fromByteBuffer(removed_key),
            [this, &removed_nodes](auto &parent, auto child_idx, auto &node)
                -> outcome::result<void> {
              OUTCOME_TRY(
                  encoded_node,
                  codec_->encodeNode(node, trie::StateVersion::V1, nullptr));
              auto node_hash = codec_->hash256(encoded_node);
              removed_nodes.insert(node_hash);
              // leaf = node.children[child_idx].get();
              return outcome::success();
            }));
        OUTCOME_TRY(encoded_node,
                    codec_->encodeNode(*leaf, trie::StateVersion::V1, nullptr));
        auto node_hash = codec_->hash256(encoded_node);
        removed_nodes.insert(node_hash);
      }
      for (auto &entry : update.inserted_keys) {
      }

      if (updates_.empty()) {
        base_block_ = update.finalized_block;
      }
      updates_.emplace_back(std::move(update));

      return outcome::success();
    }

   private:
    std::vector<TrieStateUpdate> updates_;
    primitives::BlockNumber base_block_;
    std::unordered_map<trie::KeyNibbles, primitives::BlockNumber>
        block_where_node_was_removed_;
    std::shared_ptr<trie::PolkadotCodec> codec_;
  };

  outcome::result<void> forSubtree(
      trie::TrieNode const &root,
      const trie::TrieSerializer &serializer,
      std::function<outcome::result<void>(trie::TrieNode const &)> const
          &callback) {
    struct QueueEntry {
      const trie::TrieNode *node;
      uint8_t next_child_idx = 0;
    };
    std::queue<QueueEntry> queued_nodes;
    queued_nodes.push({&root, 0});
    while (!queued_nodes.empty()) {
      auto entry = queued_nodes.back();
      OUTCOME_TRY(callback(*entry.node));
      if (entry.node->isBranch()) {
        auto branch = dynamic_cast<const trie::BranchNode *>(entry.node);
        auto next_child_idx = branch->getNextChildIdxFrom(entry.next_child_idx);
        if (next_child_idx < trie::BranchNode::kMaxChildren) {
          auto &opaque_child = branch->children[next_child_idx];
          OUTCOME_TRY(child, serializer.retrieveNode(opaque_child));
          queued_nodes.push({child.get(), next_child_idx});
        } else {
          queued_nodes.pop();
        }
      } else {
        queued_nodes.pop();
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
      OUTCOME_TRY(trie, serializer_->retrieveTrie(state));
      return forSubtree(
          *trie->getRoot(),
          *serializer_,
          [this](auto &node) -> outcome::result<void> {
            OUTCOME_TRY(
                enc, codec_->encodeNode(node, trie::StateVersion::V1, nullptr));
            auto hash = codec_->hash256(enc);
            ref_count_[hash]++;
            return outcome::success();
          });
    }

    virtual outcome::result<void> prune(trie::RootHash const &state) override {
      OUTCOME_TRY(trie, serializer_->retrieveTrie(state));
      return forSubtree(
          *trie->getRoot(),
          *serializer_,
          [this](auto &node) -> outcome::result<void> {
            OUTCOME_TRY(
                enc, codec_->encodeNode(node, trie::StateVersion::V1, nullptr));
            auto hash = codec_->hash256(enc);
            ref_count_[hash]--;
            return outcome::success();
          });
    }

   private:
    std::unordered_map<common::Hash256, size_t> ref_count_;
    std::shared_ptr<storage::trie::TrieStorageBackend> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
    std::shared_ptr<storage::trie::PolkadotCodec> codec_;
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIE_PRUNER_IMPL_HPP
