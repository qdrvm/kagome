/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie_pruner/impl/trie_pruner_impl.hpp"

#include <queue>

#include <fmt/std.h>
#include <boost/assert.hpp>

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "storage/database_error.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/spaced_storage.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_storage_backend.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie_pruner,
                            TriePrunerImpl::Error,
                            e) {
  using E = kagome::storage::trie_pruner::TriePrunerImpl::Error;
  switch (e) {
    case E::LAST_PRUNED_BLOCK_IS_LAST_FINALIZED:
      return "Last pruned block is the last finalized block, so the trie "
             "pruner cannot register the next block state";
  }
  return "Unknown TriePruner error";
}

namespace kagome::storage::trie_pruner {

  template <typename F,
            std::enable_if_t<std::is_invocable_r_v<outcome::result<void>,
                                                   F,
                                                   common::BufferView,
                                                   const trie::RootHash &>,
                             bool> = true>
  outcome::result<void> forEachChildTrie(const trie::PolkadotTrie &parent,
                                         const F &f) {
    auto child_tries = parent.trieCursor();
    OUTCOME_TRY(child_tries->seekLowerBound(storage::kChildStoragePrefix));
    while (child_tries->isValid()
           && startsWith(child_tries->key().value(),
                         storage::kChildStoragePrefix)) {
      auto child_key = child_tries->value().value();
      OUTCOME_TRY(child_hash, trie::RootHash::fromSpan(child_key));
      OUTCOME_TRY(f(child_key.view(), child_hash));
      OUTCOME_TRY(child_tries->next());
    }
    return outcome::success();
  }

  TriePrunerImpl::TriePrunerImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<storage::trie::TrieStorageBackend> node_storage,
      std::shared_ptr<const storage::trie::TrieSerializer> serializer,
      std::shared_ptr<const storage::trie::Codec> codec,
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<const crypto::Hasher> hasher,
      std::shared_ptr<const application::AppConfiguration> config)
      : node_storage_{node_storage},
        serializer_{serializer},
        codec_{codec},
        storage_{storage},
        hasher_{hasher},
        pruning_depth_{config->statePruningDepth()},
        thorough_pruning_{config->enableThoroughPruning()} {
    BOOST_ASSERT(node_storage_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

    app_state_manager->takeControl(*this);
  }

  bool TriePrunerImpl::prepare() {
    std::unique_lock lock{mutex_};
    BOOST_ASSERT(storage_->getSpace(kDefault));
    auto encoded_info_res =
        storage_->getSpace(kDefault)->tryGet(TRIE_PRUNER_INFO_KEY);
    if (!encoded_info_res) {
      SL_ERROR(logger_, "Failed to obtain trie pruner metadata");
      return false;
    }
    auto &encoded_info = encoded_info_res.value();

    if (encoded_info.has_value()) {
      if (auto info_res = scale::decode<TriePrunerInfo>(*encoded_info);
          info_res.has_value()) {
        auto &info = info_res.value();
        last_pruned_block_ = info.last_pruned_block;
      } else {
        SL_ERROR(logger_, "Failed to decode pruner info: {}", info_res.error());
        return false;
      }
    }
    SL_DEBUG(
        logger_,
        "Initialize trie pruner with pruning depth {}, last pruned block {}",
        pruning_depth_,
        last_pruned_block_);
    return true;
  }

  class EncoderCache {
   public:
    explicit EncoderCache(const trie::Codec &codec, log::Logger logger)
        : codec{codec}, logger{logger} {}

    ~EncoderCache() {
      SL_DEBUG(logger, "Encode called {} times", encode_called);
    }

    trie::MerkleValue getMerkleValue(const trie::DummyNode &node) const {
      return node.db_key;
    }

    std::optional<common::Hash256> getValueHash(const trie::TrieNode &node,
                                                trie::StateVersion version) {
      if (node.getValue().is_some()) {
        if (node.getValue().hash.has_value()) {
          return *node.getValue().hash;
        }
        if (codec.shouldBeHashed(node.getValue(), version)) {
          return codec.hash256(*node.getValue().value);
        }
      }
      return std::nullopt;
    }

    outcome::result<trie::MerkleValue> getMerkleValue(
        const trie::TrieNode &node, trie::StateVersion version) {
      encode_called++;
      // TODO(Harrm): cache is broken and thus temporarily disabled
      OUTCOME_TRY(
          merkle_value,
          codec.merkleValue(
              node,
              version,
              [this](trie::Codec::Visitee visitee) -> outcome::result<void> {
                if (auto child_data =
                        std::get_if<trie::Codec::ChildData>(&visitee);
                    child_data != nullptr) {
                  return visitChild(child_data->child,
                                    child_data->merkle_value);
                }
                return outcome::success();
              }));
      return merkle_value;
    }

   private:
    const trie::Codec &codec;
    size_t encode_called = 0;
    log::Logger logger;

    outcome::result<void> visitChild(const trie::TrieNode &node,
                                     const trie::MerkleValue &merkle_value) {
      if (merkle_value.isHash()) {
        encode_called++;
      }
      return outcome::success();
    }
  };

  outcome::result<void> TriePrunerImpl::pruneFinalized(
      const primitives::BlockHeader &block) {
    std::unique_lock lock{mutex_};
    auto node_batch = node_storage_->batch();
    OUTCOME_TRY(prune(*node_batch, block.state_root));
    OUTCOME_TRY(node_batch->commit());

    last_pruned_block_ = block.blockInfo();
    OUTCOME_TRY(savePersistentState());
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::pruneDiscarded(
      const primitives::BlockHeader &block) {
    std::unique_lock lock{mutex_};
    // should prune even when pruning depth is none
    auto node_batch = node_storage_->batch();
    auto value_batch = node_storage_->batch();
    OUTCOME_TRY(prune(*node_batch, block.state_root));
    OUTCOME_TRY(node_batch->commit());
    OUTCOME_TRY(value_batch->commit());
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::prune(BufferBatch &node_batch,
                                              const trie::RootHash &root_hash) {
    auto trie_res = serializer_->retrieveTrie(root_hash, nullptr);
    if (trie_res.has_error()
        && trie_res.error() == storage::DatabaseError::NOT_FOUND) {
      SL_TRACE(logger_,
               "Failed to obtain trie from storage, the state {} is probably "
               "already pruned or has never been executed.",
               root_hash);
      return outcome::success();
    }
    KAGOME_PROFILE_START_L(logger_, prune_state);

    OUTCOME_TRY(trie, trie_res);
    if (trie->getRoot() == nullptr) {
      SL_DEBUG(logger_, "Attempt to prune a trie with a null root");
      return outcome::success();
    }

    OUTCOME_TRY(
        forEachChildTrie(*trie,
                         [this, &node_batch](common::BufferView child_key,
                                             const trie::RootHash &child_hash) {
                           return prune(node_batch, child_hash);
                         }));

    size_t nodes_removed = 0;
    size_t values_removed = 0;
    size_t nodes_unknown = 0;
    size_t values_unknown = 0;

    struct Entry {
      common::Hash256 hash;
      std::shared_ptr<trie::TrieNode> node;
      size_t depth;
    };
    std::vector<Entry> queued_nodes;
    queued_nodes.push_back({root_hash, trie->getRoot(), 0});

    EncoderCache encoder{*codec_, logger_};

    logger_->debug("Prune state root {}", root_hash);

    // iterate nodes, decrement their ref count and delete if ref count becomes
    // zero
    while (!queued_nodes.empty()) {
      auto [hash, node, depth] = queued_nodes.back();
      queued_nodes.pop_back();
      auto ref_count_it = ref_count_.find(hash);
      if (ref_count_it == ref_count_.end()) {
        nodes_unknown++;
        continue;
      }

      auto &ref_count = ref_count_it->second;
      if (ref_count == 0) {
        SL_WARN(logger_,
                "Pruner encountered an unindexed node {} while pruning, this "
                "indicates a bug",
                hash);
        continue;
      }
      ref_count--;
      SL_TRACE(logger_,
               "Prune - {} - Node {}, ref count {}",
               depth,
               hash,
               ref_count);

      if (immortal_nodes_.find(hash) == immortal_nodes_.end()
          && ref_count == 0) {
        nodes_removed++;
        ref_count_.erase(ref_count_it);
        OUTCOME_TRY(node_batch.remove(hash));
        auto hash_opt = node->getValue().hash;
        if (hash_opt.has_value()) {
          auto &value_hash = *hash_opt;
          auto value_ref_it = value_ref_count_.find(value_hash);
          if (value_ref_it == value_ref_count_.end()) {
            values_unknown++;
          } else {
            auto &value_ref_count = value_ref_it->second;
            value_ref_count--;
            if (value_ref_count == 0) {
              OUTCOME_TRY(node_batch.remove(value_hash));
              value_ref_count_.erase(value_ref_it);
              values_removed++;
            }
          }
        }
        if (node->isBranch()) {
          auto branch = static_cast<const trie::BranchNode &>(*node);
          for (auto opaque_child : branch.children) {
            if (opaque_child != nullptr) {
              auto dummy_child =
                  dynamic_cast<const trie::DummyNode *>(opaque_child.get());
              std::optional<trie::MerkleValue> child_merkle_value;
              if (dummy_child != nullptr) {
                child_merkle_value = encoder.getMerkleValue(*dummy_child);
              } else {
                // used for tests
                auto node =
                    dynamic_cast<const trie::TrieNode *>(opaque_child.get());
                BOOST_ASSERT(node != nullptr);
                BOOST_OUTCOME_TRY(
                    child_merkle_value,
                    encoder.getMerkleValue(*node, trie::StateVersion::V0));
              }
              BOOST_ASSERT(child_merkle_value.has_value());
              if (child_merkle_value->isHash()) {
                SL_TRACE(logger_,
                         "Prune - Child {}",
                         child_merkle_value->asBuffer());
                OUTCOME_TRY(child,
                            serializer_->retrieveNode(opaque_child, nullptr));
                queued_nodes.push_back(
                    {*child_merkle_value->asHash(), child, depth + 1});
              }
            }
          }
        }
      }
    }

    SL_DEBUG(logger_, "Removed {} nodes", nodes_removed);
    if (nodes_unknown > 0) {
      SL_WARN(logger_,
              "Pruner detected {} unknown nodes during pruning. This indicates "
              "a bug.",
              nodes_unknown);
    }
    SL_DEBUG(logger_, "Removed {} values", values_removed);
    if (values_unknown > 0) {
      SL_WARN(logger_,
              "Pruner detected {} unknown nodes during pruning. This indicates "
              "a bug.",
              values_unknown);
    }

    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::addNewState(
      const storage::trie::RootHash &state_root, trie::StateVersion version) {
    std::unique_lock lock{mutex_};
    OUTCOME_TRY(trie, serializer_->retrieveTrie(state_root));
    OUTCOME_TRY(addNewStateWith(*trie, version));
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::addNewState(
      const trie::PolkadotTrie &new_trie, trie::StateVersion version) {
    std::unique_lock lock{mutex_};
    OUTCOME_TRY(addNewStateWith(new_trie, version));
    return outcome::success();
  }

  outcome::result<storage::trie::RootHash> TriePrunerImpl::addNewStateWith(
      const trie::PolkadotTrie &new_trie, trie::StateVersion version) {
    if (new_trie.getRoot() == nullptr) {
      SL_DEBUG(logger_, "Attempt to add a trie with a null root");
      return outcome::success();
    }

    SL_DEBUG(logger_, "Ref count map size is {}", ref_count_.size());
    KAGOME_PROFILE_START_L(logger_, register_state);

    struct Entry {
      std::shared_ptr<const trie::TrieNode> node;
      common::Hash256 hash;
    };
    std::vector<Entry> queued_nodes;

    EncoderCache encoder{*codec_, logger_};

    OUTCOME_TRY(root_hash,
                encoder.getMerkleValue(*new_trie.getRoot(), version));
    BOOST_ASSERT(root_hash.isHash());
    SL_DEBUG(logger_, "Add new state with hash: {}", root_hash.asBuffer());
    queued_nodes.push_back({new_trie.getRoot(), *root_hash.asHash()});

    size_t referenced_nodes_num = 0;
    size_t referenced_values_num = 0;

    while (!queued_nodes.empty()) {
      auto [node, hash] = queued_nodes.back();
      queued_nodes.pop_back();
      auto &ref_count = ref_count_[hash];
      if (ref_count == 0 && !thorough_pruning_) {
        OUTCOME_TRY(hash_is_in_storage, node_storage_->contains(hash));
        if (hash_is_in_storage) {
          // the node is present in storage but pruner has not indexed it
          // because pruner has been initialized on a newer state
          SL_TRACE(
              logger_,
              "Node {} is unindexed, but already in storage, make it immortal",
              hash.toHex());
          ref_count++;
          immortal_nodes_.emplace(hash);
        }
      }
      ref_count++;
      SL_TRACE(logger_, "Add node {}, ref count {}", hash.toHex(), ref_count);

      referenced_nodes_num++;
      bool is_new_node_with_value =
          node != nullptr && ref_count == 1 && node->getValue().is_some();
      if (is_new_node_with_value) {
        auto value_hash_opt = encoder.getValueHash(*node, version);
        if (value_hash_opt) {
          auto &value_ref_count = value_ref_count_[*value_hash_opt];
          OUTCOME_TRY(contains_value, node_storage_->contains(*value_hash_opt));
          if (value_ref_count == 0 && contains_value && !thorough_pruning_) {
            value_ref_count++;
          }
          value_ref_count++;
          referenced_values_num++;
        }
      }

      bool is_new_branch_node =
          node != nullptr && node->isBranch() && ref_count == 1;
      if (is_new_branch_node) {
        auto branch = static_cast<const trie::BranchNode *>(node.get());
        for (auto opaque_child : branch->children) {
          if (opaque_child != nullptr) {
            OUTCOME_TRY(child, serializer_->retrieveNode(opaque_child));
            OUTCOME_TRY(child_merkle_val,
                        encoder.getMerkleValue(*child, version));
            // otherwise it is not stored as a separated node, but as a part of
            // the branch
            if (child_merkle_val.isHash()) {
              SL_TRACE(logger_, "Queue child {}", child_merkle_val.asBuffer());
              queued_nodes.push_back({child, *child_merkle_val.asHash()});
            }
          }
        }
      }
    }
    OUTCOME_TRY(forEachChildTrie(
        new_trie,
        [this, version](
            common::BufferView child_key,
            const trie::RootHash &child_hash) -> outcome::result<void> {
          OUTCOME_TRY(trie, serializer_->retrieveTrie(child_hash));
          OUTCOME_TRY(addNewStateWith(*trie, version));
          return outcome::success();
        }));
    SL_DEBUG(logger_,
             "Referenced {} nodes and {} values. Ref count map size: {}",
             referenced_nodes_num,
             referenced_values_num,
             ref_count_.size());
    return *root_hash.asHash();
  }

  outcome::result<void> TriePrunerImpl::recoverState(
      const blockchain::BlockTree &block_tree) {
    std::unique_lock lock{mutex_};
    static log::Logger logger =
        log::createLogger("PrunerStateRecovery", "storage");
    auto last_pruned_block = last_pruned_block_;
    if (!last_pruned_block.has_value()) {
      if (block_tree.bestBlock().number != 0) {
        SL_WARN(logger,
                "Running pruner on a non-empty non-pruned storage may lead to "
                "skipping some stored states.");
        OUTCOME_TRY(
            last_finalized,
            block_tree.getBlockHeader(block_tree.getLastFinalized().hash));

        if (auto res = restoreStateAt(last_finalized, block_tree);
            res.has_error()) {
          SL_ERROR(logger,
                   "Failed to restore trie pruner state starting from last "
                   "finalized "
                   "block: {}",
                   res.error().message());
          return res.as_failure();
        }
      } else {
        OUTCOME_TRY(
            genesis_header,
            block_tree.getBlockHeader(block_tree.getGenesisBlockHash()));
        OUTCOME_TRY(trie, serializer_->retrieveTrie(genesis_header.state_root));
        OUTCOME_TRY(addNewStateWith(*trie, trie::StateVersion::V0));
      }
    } else {
      OUTCOME_TRY(base_block_header,
                  block_tree.getBlockHeader(last_pruned_block.value().hash));
      BOOST_ASSERT(block_tree.getLastFinalized().number
                   >= last_pruned_block.value().number);
      if (auto res = restoreStateAt(base_block_header, block_tree);
          res.has_error()) {
        SL_WARN(logger,
                "Failed to restore trie pruner state starting from base "
                "block {}: {}",
                last_pruned_block.value(),
                res.error().message());
      }
    }
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::restoreStateAt(
      const primitives::BlockHeader &last_pruned_block,
      const blockchain::BlockTree &block_tree) {
    KAGOME_PROFILE_START_L(logger_, restore_state);
    SL_DEBUG(logger_,
             "Restore state - last pruned block {}",
             last_pruned_block.blockInfo());

    ref_count_.clear();

    std::queue<primitives::BlockHash> block_queue;

    OUTCOME_TRY(last_pruned_children,
                block_tree.getChildren(last_pruned_block.hash()));
    if (!last_pruned_children.empty()) {
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
      OUTCOME_TRY(addNewStateWith(*base_tree, trie::StateVersion::V0));
      OUTCOME_TRY(children, block_tree.getChildren(base_block_hash));
      for (auto child : children) {
        block_queue.push(child);
      }
    }

    while (!block_queue.empty()) {
      auto block_hash = block_queue.front();
      block_queue.pop();

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
        continue;
      }
      OUTCOME_TRY(tree, tree_res);
      OUTCOME_TRY(addNewStateWith(*tree, trie::StateVersion::V0));

      OUTCOME_TRY(children, block_tree.getChildren(block_hash));
      for (auto child : children) {
        block_queue.push(child);
      }
    }
    last_pruned_block_ = last_pruned_block.blockInfo();
    OUTCOME_TRY(savePersistentState());
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::savePersistentState() const {
    OUTCOME_TRY(enc_info,
                scale::encode(TriePrunerInfo{
                    last_pruned_block_,
                }));
    BOOST_ASSERT(storage_->getSpace(kDefault));
    OUTCOME_TRY(storage_->getSpace(kDefault)->put(
        TRIE_PRUNER_INFO_KEY, common::Buffer{std::move(enc_info)}));
    return outcome::success();
  }

  void TriePrunerImpl::restoreStateAtFinalized(
      const blockchain::BlockTree &block_tree) {
    std::unique_lock lock{mutex_};
    auto header_res =
        block_tree.getBlockHeader(block_tree.getLastFinalized().hash);
    if (header_res.has_error()) {
      SL_ERROR(logger_,
               "restoreStateAtFinalized(): getBlockHeader(): {}",
               header_res.error());
      return;
    }
    auto &header = header_res.value();
    if (auto r = restoreStateAt(header, block_tree); r.has_error()) {
      SL_ERROR(logger_,
               "restoreStateAtFinalized(): restoreStateAt(): {}",
               r.error());
    }
  }
}  // namespace kagome::storage::trie_pruner
