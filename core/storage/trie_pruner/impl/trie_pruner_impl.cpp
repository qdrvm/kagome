/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie_pruner/impl/trie_pruner_impl.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie_pruner,
                            TriePrunerImpl::Error,
                            e) {
  using E = kagome::storage::trie_pruner::TriePrunerImpl::Error;
  switch (e) {
    case E::OUTDATED_PRUNE_BASE:
      return "Attempt to create trie pruner on a database with outdated pruner "
             "info record (most likely a corrupted database)";
    case E::CREATE_PRUNER_ON_NON_PRUNED_NON_EMPTY_STORAGE:
      return "Attempt to create trie pruner on a non-pruned non-empty database";
  }
  return "Unknown TriePruner error";
}

namespace kagome::storage::trie_pruner {

  static outcome::result<common::Buffer> calcMerkleValue(
      const trie::Codec &codec,
      const trie::OpaqueTrieNode &node,
      trie::StateVersion version) {
    static log::Logger logger;
    if (logger == nullptr) {
      logger = log::createLogger("PRUNER");
    }
    OUTCOME_TRY(
        hash,
        codec.merkleValue(node, version));
    return hash;
  }

  static outcome::result<common::Buffer> calcHash(const trie::Codec &codec,
                                                  const trie::TrieNode &node,
                                                  trie::StateVersion version) {
    OUTCOME_TRY(
        enc,
        codec.encodeNode(
            node, version, [](auto &, auto, auto &&) -> outcome::result<void> {
              return outcome::success();
            }));
    auto hash = codec.hash256(enc);
    return hash;
  }

  outcome::result<void> TriePrunerImpl::init(
      std::shared_ptr<blockchain::BlockStorage> block_storage) {
    OUTCOME_TRY(encoded_info,
                storage_->getSpace(kTriePruner)->tryGet(TRIE_PRUNER_INFO_KEY));

    if (!encoded_info) {
      OUTCOME_TRY(header, block_storage->getBlockHeader(1));
      if (header.has_value()) {
        return Error::CREATE_PRUNER_ON_NON_PRUNED_NON_EMPTY_STORAGE;
      }
    } else {
      OUTCOME_TRY(info, scale::decode<TriePrunerInfo>(encoded_info.value()));
      OUTCOME_TRY(finalized, block_storage->getLastFinalized());
      if (finalized.number != info.prune_base) {
        return Error::OUTDATED_PRUNE_BASE;
      }
    }
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::prune(
      primitives::BlockHeader const &state, trie::StateVersion version) {
    OUTCOME_TRY(trie, serializer_->retrieveTrie(state.state_root, nullptr));
    size_t removed = 0;
    size_t unknown = 0;

    struct Entry {
      std::shared_ptr<trie::TrieNode> node;
      size_t depth;
    };
    std::vector<Entry> queued_nodes;
    queued_nodes.push_back({trie->getRoot(), 0});

    OUTCOME_TRY(root_value,
                calcMerkleValue(*codec_, *trie->getRoot(), version));
    OUTCOME_TRY(root_hash, calcHash(*codec_, *trie->getRoot(), version));

    logger_->info("Prune: Merkle value of state {} (#{} - state_root {}) is {}",
                  root_hash,
                  state.number,
                  state.state_root,
                  root_value);

    auto batch = trie_storage_->batch();
    // iterate nodes, decrement their ref count and delete if ref count becomes
    // zero
    while (!queued_nodes.empty()) {
      auto [node, depth] = queued_nodes.back();
      queued_nodes.pop_back();
      // FIXME: crutch because root nodes are always hashed, so encoding
      // doesn't match here and in addState (which always just takes merkle
      // value)
      OUTCOME_TRY(merkle_value, calcMerkleValue(*codec_, *node, version));
      auto hash = merkle_value;
      auto ref_count_it = ref_count_.find(hash);
      if (ref_count_it == ref_count_.end()) {
        unknown++;
        continue;
      }

      auto &ref_count = ref_count_it->second;
      BOOST_ASSERT(ref_count != 0);
      ref_count--;
      SL_TRACE(logger_,
               "Prune - {} - Node {}, ref count {}",
               depth,
               hash,
               ref_count);

      if (ref_count == 0) {
        removed++;
        ref_count_.erase(hash);
        OUTCOME_TRY(batch->remove(hash));
        if (node->isBranch()) {
          auto branch = static_cast<const trie::BranchNode &>(*node);
          for (auto opaque_child : branch.children) {
            if (opaque_child != nullptr) {
              OUTCOME_TRY(hash,
                          calcMerkleValue(*codec_, *opaque_child, version));
              SL_TRACE(logger_, "Prune - Child {}", hash);
              OUTCOME_TRY(child,
                          serializer_->retrieveNode(opaque_child, nullptr));
              queued_nodes.push_back({child, depth + 1});
            }
          }
        }
      }
    }

    OUTCOME_TRY(batch->commit());
    OUTCOME_TRY(enc_info, scale::encode(TriePrunerInfo{state.number}));
    OUTCOME_TRY(
        storage_->getSpace(kTriePruner)
            ->put(TRIE_PRUNER_INFO_KEY, common::Buffer{std::move(enc_info)}));

    SL_DEBUG(logger_, "Removed {} nodes, {} unknown", removed, unknown);
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::addNewState(
      trie::PolkadotTrie const &new_trie, trie::StateVersion version) {
    SL_DEBUG(logger_, "Ref count map size is {}", ref_count_.size());
    KAGOME_PROFILE_START_L(logger_, register_state);

    std::vector<std::shared_ptr<const trie::OpaqueTrieNode>> queued_nodes;
    queued_nodes.push_back(new_trie.getRoot());

    OUTCOME_TRY(root_value,
                calcMerkleValue(*codec_, *new_trie.getRoot(), version));
    OUTCOME_TRY(root_hash, calcHash(*codec_, *new_trie.getRoot(), version));
    ref_count_[root_value] += 1;
    logger_->info("Add: Merkle value of state {} is {}", root_hash, root_value);

    size_t referenced_nodes_num = 0;

    while (!queued_nodes.empty()) {
      auto opaque_node = queued_nodes.back();
      queued_nodes.pop_back();
      // we encode nodes thrice: here, when visiting it in the for loop below,
      // and during serialization to DB. Should avoid it somehow. Mind that
      // encode node's callback doesn't report dummy nodes
      OUTCOME_TRY(hash, calcMerkleValue(*codec_, *opaque_node, version));
      auto ref_count = ref_count_[hash];
      SL_TRACE(logger_, "Add - Node {}, ref count {}", hash.toHex(), ref_count);

      referenced_nodes_num++;
      if (auto node = dynamic_cast<trie::TrieNode const *>(opaque_node.get());
          node != nullptr && node->isBranch() && ref_count_[hash] == 1) {
        auto branch = static_cast<const trie::BranchNode *>(opaque_node.get());
        for (auto opaque_child : branch->children) {
          if (opaque_child != nullptr) {
            OUTCOME_TRY(child_hash,
                        calcMerkleValue(*codec_, *opaque_child, version));
            ref_count_[child_hash] += 1;
            SL_TRACE(logger_, "Add - Child {}", child_hash.toHex());
            queued_nodes.push_back(opaque_child);
          }
        }
      }
    }
    //OUTCOME_TRY(printTree(new_trie, version));
    SL_DEBUG(logger_,
            "Referenced {} nodes\nRef count map size: {}",
            referenced_nodes_num,
            ref_count_.size());
    return outcome::success();
  }

}  // namespace kagome::storage::trie_pruner
