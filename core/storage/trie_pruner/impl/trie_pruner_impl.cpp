/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie_pruner/impl/trie_pruner_impl.hpp"

#include <boost/assert.hpp>
#include <queue>

#include "application/app_configuration.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "storage/database_error.hpp"
#include "storage/spaced_storage.hpp"
#include "storage/spaces.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_storage_backend.hpp"

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
    case E::LAST_PRUNED_BLOCK_IS_LAST_FINALIZED:
      return "Last pruned block is the last finalized block, so the trie "
             "pruner cannot register the next block state";
  }
  return "Unknown TriePruner error";
}

namespace kagome::storage::trie_pruner {

  outcome::result<std::unique_ptr<TriePrunerImpl>> TriePrunerImpl::create(
      std::shared_ptr<const application::AppConfiguration> config,
      std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage,
      std::shared_ptr<const storage::trie::TrieSerializer> serializer,
      std::shared_ptr<const storage::trie::Codec> codec,
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<const crypto::Hasher> hasher) {
    auto pruner = std::unique_ptr<TriePrunerImpl>(
        new TriePrunerImpl{config->statePruningDepth(),
                           trie_storage,
                           serializer,
                           codec,
                           storage,
                           hasher});
    BOOST_ASSERT(storage->getSpace(kTriePruner));
    OUTCOME_TRY(encoded_info,
                storage->getSpace(kTriePruner)->tryGet(TRIE_PRUNER_INFO_KEY));
    if (encoded_info.has_value()) {
      if (auto info_res = scale::decode<TriePrunerInfo>(*encoded_info);
          info_res.has_value()) {
        auto &info = info_res.value();
        pruner->last_pruned_block_ = info.last_pruned_block;
        pruner->child_states_.insert(info.child_states.begin(),
                                     info.child_states.end());
      } else {
        SL_ERROR(pruner->logger_,
                 "Failed to decode pruner info: {}",
                 info_res.error());
      }
    }
    return pruner;
  }

  template <typename F>
  static outcome::result<common::Buffer> calcMerkleValue(
      const trie::Codec &codec,
      const trie::OpaqueTrieNode &node,
      std::optional<trie::StateVersion> version,
      const F &child_visitor) {
    OUTCOME_TRY(hash, codec.merkleValue(node, version, child_visitor));
    return hash;
  }

  static outcome::result<common::Buffer> calcHash(
      const trie::Codec &codec,
      const trie::TrieNode &node,
      std::optional<trie::StateVersion> version) {
    OUTCOME_TRY(
        enc,
        codec.encodeNode(
            node, version, [](auto &, auto, auto &&) -> outcome::result<void> {
              return outcome::success();
            }));
    auto hash = codec.hash256(enc);
    return hash;
  }

  struct EncoderCache {
    explicit EncoderCache(const trie::Codec &codec, log::Logger logger)
        : codec{codec}, logger{logger} {}

    std::unordered_map<const trie::OpaqueTrieNode *, common::Buffer> enc_cache;
    const trie::Codec &codec;
    size_t encode_called = 0;
    log::Logger logger;

    ~EncoderCache() {
      SL_DEBUG(logger, "Encode called {} times", encode_called);
    }

    outcome::result<void> visitChild(const trie::OpaqueTrieNode &node,
                                     common::BufferView merkle_value) {
      enc_cache[&node] = merkle_value;
      encode_called++;
      return outcome::success();
    }

    outcome::result<common::Buffer> getMerkleValue(
        const trie::OpaqueTrieNode &node,
        std::optional<trie::StateVersion> version) {
      encode_called++;
      bool is_branch = dynamic_cast<const trie::BranchNode *>(&node) != nullptr;
      // TODO(Harrm): cache is broken and thus temporarily disabled
      if (auto it = enc_cache.find(&node); false && it != enc_cache.end()) {
        SL_TRACE(
            logger, "Cache hit {} = {}", fmt::ptr(&node), it->second.toHex());
        OUTCOME_TRY(hash,
                    calcMerkleValue(codec,
                                    node,
                                    version,
                                    [this](auto &node, auto merkle_value, auto)
                                        -> outcome::result<void> {
                                      return visitChild(node, merkle_value);
                                      return outcome::success();
                                    }));
        BOOST_ASSERT(hash == it->second);
        return it->second;
      } else {
        OUTCOME_TRY(hash,
                    calcMerkleValue(codec,
                                    node,
                                    version,
                                    [this](auto &node, auto merkle_value, auto)
                                        -> outcome::result<void> {
                                      return visitChild(node, merkle_value);
                                      return outcome::success();
                                    }));
        // SL_TRACE(logger, "Cache miss {} = {}", fmt::ptr(&node),
        // hash.toHex());
        if (is_branch) {
          enc_cache[&node] = hash;
        }
        return hash;
      }
    }
  };

  outcome::result<void> TriePrunerImpl::init(
      const blockchain::BlockTree &block_tree) {
    SL_DEBUG(
        logger_,
        "Initialize trie pruner with pruning depth {}, last pruned block {}",
        pruning_depth_,
        last_pruned_block_);
    if (!last_pruned_block_.has_value()) {
      OUTCOME_TRY(first_hash_opt, block_tree.getBlockHash(1));
      if (first_hash_opt.has_value()) {
        SL_WARN(logger_,
                "Running pruner on a non-empty non-pruned storage may lead to "
                "skipping some stored states.");
        OUTCOME_TRY(
            last_finalized,
            block_tree.getBlockHeader(block_tree.getLastFinalized().hash));

        if (auto res = restoreState(last_finalized, block_tree);
            res.has_error()) {
          SL_ERROR(logger_,
                   "Failed to restore trie pruner state starting from last "
                   "finalized "
                   "block: {}",
                   res.error().message());
          return res;
        }
      }
    } else {
      OUTCOME_TRY(base_block_header,
                  block_tree.getBlockHeader(last_pruned_block_.value().hash));
      BOOST_ASSERT(block_tree.getLastFinalized().number
                   >= last_pruned_block_->number);
      if (auto res = restoreState(base_block_header, block_tree);
          res.has_error()) {
        SL_WARN(logger_,
                "Failed to restore trie pruner state starting from base "
                "block {}: {}",
                last_pruned_block_.value(),
                res.error().message());
        return outcome::success();
      }
    }
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::pruneFinalized(
      const primitives::BlockHeader &block) {
    OUTCOME_TRY(prune(block));

    OUTCOME_TRY(block_enc, scale::encode(block));
    auto block_hash = hasher_->blake2b_256(block_enc);

    last_pruned_block_ = primitives::BlockInfo{block_hash, block.number};
    OUTCOME_TRY(savePersistentState());
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::pruneDiscarded(
      const primitives::BlockHeader &block) {
    OUTCOME_TRY(prune(block));
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::prune(
      const primitives::BlockHeader &block) {
    auto trie_res = serializer_->retrieveTrie(block.state_root, nullptr);
    if (trie_res.has_error()
        && trie_res.error() == storage::DatabaseError::NOT_FOUND) {
      SL_TRACE(logger_,
               "Failed to obtain trie from storage, the block {} is probably "
               "already pruned "
               "or never has been executed.",
               block.number);
      return outcome::success();
    }
    KAGOME_PROFILE_START_L(logger_, prune_state);

    OUTCOME_TRY(trie, trie_res);
    if (trie->getRoot() == nullptr) {
      SL_DEBUG(logger_, "Attempt to prune a trie with a null root");
      return outcome::success();
    }

    size_t nodes_removed = 0;
    size_t values_removed = 0;
    size_t nodes_unknown = 0;
    size_t values_unknown = 0;

    struct Entry {
      std::shared_ptr<trie::TrieNode> node;
      size_t depth;
    };
    std::vector<Entry> queued_nodes;
    queued_nodes.push_back({trie->getRoot(), 0});

    EncoderCache encoder{*codec_, logger_};

    OUTCOME_TRY(root_value,
                encoder.getMerkleValue(*trie->getRoot(), std::nullopt));
    OUTCOME_TRY(root_hash, calcHash(*codec_, *trie->getRoot(), std::nullopt));

    logger_->info("Prune: Merkle value of state {} (#{} - state_root {}) is {}",
                  root_hash,
                  block.number,
                  block.state_root,
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
      OUTCOME_TRY(merkle_value, encoder.getMerkleValue(*node, std::nullopt));
      auto ref_count_it = ref_count_.find(merkle_value);
      if (ref_count_it == ref_count_.end()) {
        nodes_unknown++;
        continue;
      }

      auto &ref_count = ref_count_it->second;
      BOOST_ASSERT(ref_count != 0);
      ref_count--;
      SL_TRACE(logger_,
               "Prune - {} - Node {}, ref count {}",
               depth,
               merkle_value,
               ref_count);

      if (ref_count == 0) {
        nodes_removed++;
        ref_count_.erase(merkle_value);
        OUTCOME_TRY(batch->remove(merkle_value));
        if (node->getValue().is_some()) {
          common::Hash256 hash;
          if (node->getValue().value
              && (!node->getValue().hash
                  || codec_->shouldBeHashed(node->getValue(),
                                            trie::StateVersion::V1))) {
            hash = codec_->hash256(node->getValue().value.value());
          } else {
            hash = node->getValue().hash.value();
          }
          if (value_ref_count_[hash] == 0) {
            values_unknown++;
          } else {
            value_ref_count_[hash]--;
            if (value_ref_count_[hash] == 0) {
              OUTCOME_TRY(batch->remove(hash));
              values_removed++;
            }
          }
        }
        if (node->isBranch()) {
          auto branch = static_cast<const trie::BranchNode &>(*node);
          for (auto opaque_child : branch.children) {
            if (opaque_child != nullptr) {
              OUTCOME_TRY(child_merkle_value,
                          encoder.getMerkleValue(*opaque_child, std::nullopt));
              SL_TRACE(logger_, "Prune - Child {}", child_merkle_value);
              OUTCOME_TRY(child,
                          serializer_->retrieveNode(opaque_child, nullptr));
              queued_nodes.push_back({child, depth + 1});
            }
          }
        }
      }
    }

    OUTCOME_TRY(batch->commit());

    SL_DEBUG(
        logger_, "Removed {} nodes, {} unknown", nodes_removed, nodes_unknown);
    SL_DEBUG(logger_,
             "Removed {} values, {} unknown",
             values_removed,
             values_unknown);

    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::addNewState(
      const trie::PolkadotTrie &new_trie, trie::StateVersion version) {
    OUTCOME_TRY(addNewStateWith(new_trie,
                                version,
                                AddConfig{
                                    .type = AddConfig::AddLoadedOnly,
                                }));
    OUTCOME_TRY(savePersistentState());
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::addNewChildState(
      const storage::trie::RootHash &parent_root,
      common::BufferView key,
      const trie::PolkadotTrie &new_trie,
      trie::StateVersion version) {
    OUTCOME_TRY(child_root_hash,
                addNewStateWith(new_trie,
                                version,
                                AddConfig{
                                    .type = AddConfig::AddLoadedOnly,
                                }));
    OUTCOME_TRY(markAsChild(Parent{parent_root}, key, Child{child_root_hash}));
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::markAsChild(Parent parent,
                                                    const common::Buffer &key,
                                                    Child child) {
    child_states_[parent.hash].emplace_back(ChildStorageInfo{key, child.hash});
    OUTCOME_TRY(savePersistentState());
    return outcome::success();
  }

  outcome::result<storage::trie::RootHash> TriePrunerImpl::addNewStateWith(
      const trie::PolkadotTrie &new_trie,
      trie::StateVersion version,
      AddConfig config) {
    if (new_trie.getRoot() == nullptr) {
      SL_DEBUG(logger_, "Attempt to add a trie with a null root");
      return outcome::success();
    }

    SL_DEBUG(logger_, "Ref count map size is {}", ref_count_.size());
    KAGOME_PROFILE_START_L(logger_, register_state);

    std::vector<std::shared_ptr<const trie::OpaqueTrieNode>> queued_nodes;
    queued_nodes.push_back(new_trie.getRoot());

    EncoderCache encoder{*codec_, logger_};

    OUTCOME_TRY(root_value,
                encoder.getMerkleValue(*new_trie.getRoot(), version));
    OUTCOME_TRY(root_hash, calcHash(*codec_, *new_trie.getRoot(), version));
    SL_DEBUG(logger_, "Add new state with hash: {}", root_hash);

    ref_count_[root_value] += 1;

    size_t referenced_nodes_num = 0;
    size_t referenced_values_num = 0;

    while (!queued_nodes.empty()) {
      auto opaque_node = queued_nodes.back();
      queued_nodes.pop_back();
      OUTCOME_TRY(hash, encoder.getMerkleValue(*opaque_node, version));
      SL_TRACE(logger_,
               "Add - Node {}, ref count {}",
               hash.toHex(),
               ref_count_[hash]);

      referenced_nodes_num++;
      auto node = dynamic_cast<const trie::TrieNode *>(opaque_node.get());
      bool is_new_node_with_value = node != nullptr && ref_count_[hash] == 1
                                 && node->getValue().is_some();
      if (is_new_node_with_value) {
        common::Hash256 value_hash;
        if (node->getValue().value
            && (!node->getValue().hash
                || codec_->shouldBeHashed(node->getValue(), version))) {
          value_hash = codec_->hash256(node->getValue().value.value());
        } else {
          value_hash = node->getValue().hash.value();
        }
        value_ref_count_[value_hash]++;
        referenced_values_num++;
      }

      bool is_new_branch_node =
          node != nullptr && node->isBranch()
          && (config.shouldAddAllNodes() || ref_count_[hash] == 1);
      if (is_new_branch_node) {
        auto branch = static_cast<const trie::BranchNode *>(opaque_node.get());
        for (auto opaque_child : branch->children) {
          if (opaque_child != nullptr) {
            OUTCOME_TRY(child_hash,
                        encoder.getMerkleValue(*opaque_child, version));
            ref_count_[child_hash] += 1;
            SL_TRACE(logger_, "Add - Child {}", child_hash.toHex());
            if (config.shouldLoadDummies()) {
              OUTCOME_TRY(child, serializer_->retrieveNode(opaque_child));
              queued_nodes.push_back(child);
            } else {
              queued_nodes.push_back(opaque_child);
            }
          }
        }
      }
    }
    SL_DEBUG(logger_,
             "Referenced {} nodes and {} values. Ref count map size: {}",
             referenced_nodes_num,
             referenced_values_num,
             ref_count_.size());
    return common::Hash256::fromSpan(root_hash);
  }

  outcome::result<void> TriePrunerImpl::restoreState(
      const primitives::BlockHeader &last_pruned_block,
      const blockchain::BlockTree &block_tree) {
    KAGOME_PROFILE_START_L(logger_, restore_state);
    SL_DEBUG(logger_,
             "Restore state - last pruned block #{}",
             last_pruned_block.number);

    ref_count_.clear();
    OUTCOME_TRY(last_pruned_enc, scale::encode(last_pruned_block));
    auto last_pruned_hash = hasher_->blake2b_256(last_pruned_enc);

    OUTCOME_TRY(last_pruned_children, block_tree.getChildren(last_pruned_hash));
    if (last_pruned_children.size() > 1 || last_pruned_children.empty()) {
      return Error::LAST_PRUNED_BLOCK_IS_LAST_FINALIZED;
    }
    auto &base_block_hash = last_pruned_children.at(0);
    OUTCOME_TRY(base_block, block_tree.getBlockHeader(base_block_hash));
    auto base_tree_res = serializer_->retrieveTrie(base_block.state_root);
    if (base_tree_res.has_error()
        && base_tree_res.error() == storage::DatabaseError::NOT_FOUND) {
      SL_DEBUG(
          logger_,
          "Failed to restore pruner state, probably node is fast-syncing.");
      return outcome::success();
    }
    OUTCOME_TRY(base_tree, std::move(base_tree_res));
    OUTCOME_TRY(addNewStateWith(*base_tree,
                                trie::StateVersion::V0,
                                AddConfig{.type = AddConfig::AddWholeState}));
    std::queue<primitives::BlockHash> block_queue;

    {
      OUTCOME_TRY(children, block_tree.getChildren(base_block_hash));
      for (auto child : children) {
        block_queue.push(child);
      }
    }
    while (!block_queue.empty()) {
      auto block_hash = block_queue.front();
      OUTCOME_TRY(header, block_tree.getBlockHeader(block_hash));
      SL_DEBUG(logger_,
               "Restore state - register #{} ({})",
               header.number,
               block_hash);
      auto tree_res = serializer_->retrieveTrie(header.state_root);
      if (tree_res.has_error()
          && tree_res.error() == DatabaseError::NOT_FOUND) {
        SL_WARN(logger_,
                "State for block #{} is not found in the database",
                header.number);
        block_queue.pop();
        continue;
      }
      OUTCOME_TRY(tree, tree_res);
      OUTCOME_TRY(
          addNewStateWith(*tree,
                          trie::StateVersion::V0,
                          AddConfig{.type = AddConfig::AddNewLoadDummies}));

      block_queue.pop();
      OUTCOME_TRY(children, block_tree.getChildren(block_hash));
      for (auto child : children) {
        block_queue.push(child);
      }
    }
    last_pruned_block_ = {last_pruned_hash, last_pruned_block.number};
    OUTCOME_TRY(savePersistentState());
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::savePersistentState() const {
    std::vector<std::pair<primitives::BlockHash, std::vector<ChildStorageInfo>>>
        child_states;
    std::copy(child_states_.begin(),
              child_states_.end(),
              std::back_inserter(child_states));

    OUTCOME_TRY(enc_info,
                scale::encode(TriePrunerInfo{
                    last_pruned_block_,
                    child_states,
                }));
    BOOST_ASSERT(storage_->getSpace(kTriePruner));
    OUTCOME_TRY(
        storage_->getSpace(kTriePruner)
            ->put(TRIE_PRUNER_INFO_KEY, common::Buffer{std::move(enc_info)}));
    return outcome::success();
  }

}  // namespace kagome::storage::trie_pruner
