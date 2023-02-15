/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_PRUNER_IMPL_HPP
#define KAGOME_TRIE_PRUNER_IMPL_HPP

#include "storage/trie_pruner/trie_pruner.hpp"

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
            [this, &removed_nodes, &leaf](
                auto &node, auto child_idx) -> outcome::result<void> {
              OUTCOME_TRY(
                  encoded_node,
                  codec_->encodeNode(node, trie::StateVersion::V1, nullptr));
              auto node_hash = codec_->hash256(encoded_node);
              removed_nodes.insert(node_hash);
              leaf = node.children[child_idx].get();
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

  class TriePrunerImpl : public TriePruner {
   public:
    virtual outcome::result<void> addNewState(
        TrieStateUpdate const &update) override {
      OUTCOME_TRY(trie, serializer_->retrieveTrie(update.old_storage_root));
      return prune_window_.addUpdate(*trie, update);
    }

    virtual void prune(primitives::BlockNumber last_finalized) override {}

   private:
    StateWindow prune_window_;
    std::shared_ptr<storage::trie::TrieStorageBackend> storage_;
    std::shared_ptr<storage::trie::TrieSerializer> serializer_;
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIE_PRUNER_IMPL_HPP
