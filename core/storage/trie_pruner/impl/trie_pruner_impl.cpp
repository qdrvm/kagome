/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie_pruner/impl/trie_pruner_impl.hpp"

#include <queue>

#include <boost/assert.hpp>

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "storage/database_error.hpp"
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

  TriePrunerImpl::TriePrunerImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<storage::trie::TrieStorageBackend> trie_storage,
      std::shared_ptr<const storage::trie::TrieSerializer> serializer,
      std::shared_ptr<const storage::trie::Codec> codec,
      std::shared_ptr<storage::SpacedStorage> storage,
      std::shared_ptr<const crypto::Hasher> hasher,
      std::shared_ptr<const application::AppConfiguration> config)
      : trie_storage_{trie_storage},
        serializer_{serializer},
        codec_{codec},
        storage_{storage},
        hasher_{hasher},
        pruning_depth_{config->statePruningDepth()} {
    BOOST_ASSERT(trie_storage_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);

    app_state_manager->takeControl(*this);
  }

  bool TriePrunerImpl::prepare() {
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
      if (auto it = enc_cache.find(&node); false && it != enc_cache.end()) {
        SL_TRACE(
            logger, "Cache hit {} = {}", fmt::ptr(&node), it->second.toHex());
        return it->second;
      }

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
      if (merkle_value.isHash()) {
        enc_cache[&node] = *merkle_value.asHash();
      }
      return merkle_value;
    }

   private:
    std::unordered_map<const trie::TrieNode *, common::Hash256> enc_cache;
    const trie::Codec &codec;
    size_t encode_called = 0;
    log::Logger logger;

    outcome::result<void> visitChild(const trie::TrieNode &node,
                                     const trie::MerkleValue &merkle_value) {
      if (merkle_value.isHash()) {
        enc_cache[&node] = *merkle_value.asHash();
        encode_called++;
      }
      return outcome::success();
    }
  };

  outcome::result<void> TriePrunerImpl::pruneFinalized(
      const primitives::BlockHeader &block) {
    OUTCOME_TRY(prune(block.state_root));

    OUTCOME_TRY(block_enc, scale::encode(block));
    auto block_hash = hasher_->blake2b_256(block_enc);

    last_pruned_block_ = primitives::BlockInfo{block_hash, block.number};
    OUTCOME_TRY(savePersistentState());
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::pruneDiscarded(
      const primitives::BlockHeader &block) {
    // should prune even when pruning depth is none
    OUTCOME_TRY(prune(block.state_root));
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::prune(
      const trie::RootHash &root_hash) {
    auto trie_res = serializer_->retrieveTrie(root_hash, nullptr);
    if (trie_res.has_error()
        && trie_res.error() == storage::DatabaseError::NOT_FOUND) {
      SL_TRACE(logger_,
               "Failed to obtain trie from storage, the state {} is probably "
               "already pruned "
               "or never has been executed.",
               root_hash);
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
      common::Hash256 hash;
      std::shared_ptr<trie::TrieNode> node;
      size_t depth;
    };
    std::vector<Entry> queued_nodes;
    queued_nodes.push_back({root_hash, trie->getRoot(), 0});

    EncoderCache encoder{*codec_, logger_};

    logger_->debug("Prune state root {}", root_hash);

    auto batch = trie_storage_->batch();

    std::scoped_lock lock{ref_count_mutex_};

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
      BOOST_ASSERT(ref_count != 0);
      ref_count--;
      SL_TRACE(logger_,
               "Prune - {} - Node {}, ref count {}",
               depth,
               hash,
               ref_count);

      if (ref_count == 0) {
        nodes_removed++;
        ref_count_.erase(ref_count_it);
        OUTCOME_TRY(batch->remove(hash));
        auto hash_opt = node->getValue().hash;
        if (hash_opt.has_value()) {
          auto &value_ref_count = value_ref_count_[hash];
          if (value_ref_count == 0) {
            values_unknown++;
          } else {
            value_ref_count--;
            if (value_ref_count == 0) {
              OUTCOME_TRY(batch->remove(hash));
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
                OUTCOME_TRY(
                    res, encoder.getMerkleValue(*node, trie::StateVersion::V0));
                child_merkle_value = std::move(res);
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

    OUTCOME_TRY(batch->commit());

    OUTCOME_TRY(pruneChildStates(*trie));

    SL_DEBUG(
        logger_, "Removed {} nodes, {} unknown", nodes_removed, nodes_unknown);
    SL_DEBUG(logger_,
             "Removed {} values, {} unknown",
             values_removed,
             values_unknown);

    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::addNewState(
      const storage::trie::RootHash &state_root, trie::StateVersion version) {
    OUTCOME_TRY(trie, serializer_->retrieveTrie(state_root));
    OUTCOME_TRY(addNewStateWith(*trie, version));
    OUTCOME_TRY(savePersistentState());
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::addNewState(
      const trie::PolkadotTrie &new_trie, trie::StateVersion version) {
    OUTCOME_TRY(addNewStateWith(new_trie, version));
    OUTCOME_TRY(savePersistentState());
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

    std::scoped_lock lock{ref_count_mutex_};

    ref_count_[*root_hash.asHash()] += 1;

    size_t referenced_nodes_num = 0;
    size_t referenced_values_num = 0;

    while (!queued_nodes.empty()) {
      auto [node, hash] = queued_nodes.back();
      queued_nodes.pop_back();
      auto ref_count = ref_count_[hash];
      SL_TRACE(logger_, "Add - Node {}, ref count {}", hash.toHex(), ref_count);

      referenced_nodes_num++;
      bool is_new_node_with_value =
          node != nullptr && ref_count == 1 && node->getValue().is_some();
      if (is_new_node_with_value) {
        auto value_hash_opt = encoder.getValueHash(*node, version);
        if (value_hash_opt) {
          value_ref_count_[*value_hash_opt]++;
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
            if (child_merkle_val.isHash()) {
              ref_count_[*child_merkle_val.asHash()] += 1;
              SL_TRACE(logger_, "Add - Child {}", child_merkle_val.asBuffer());
              queued_nodes.push_back({child, *child_merkle_val.asHash()});
            }
          }
        }
      }
    }
    OUTCOME_TRY(addChildStates(new_trie, *root_hash.asHash()));
    SL_DEBUG(logger_,
             "Referenced {} nodes and {} values. Ref count map size: {}",
             referenced_nodes_num,
             referenced_values_num,
             ref_count_.size());
    return *root_hash.asHash();
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

    std::queue<primitives::BlockHash> block_queue;

    OUTCOME_TRY(last_pruned_children, block_tree.getChildren(last_pruned_hash));
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
      OUTCOME_TRY(addNewStateWith(*tree, trie::StateVersion::V0));

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
    OUTCOME_TRY(enc_info,
                scale::encode(TriePrunerInfo{
                    last_pruned_block_,
                }));
    BOOST_ASSERT(storage_->getSpace(kDefault));
    OUTCOME_TRY(storage_->getSpace(kDefault)->put(
        TRIE_PRUNER_INFO_KEY, common::Buffer{std::move(enc_info)}));
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::addChildStates(
      const trie::PolkadotTrie &parent, const trie::RootHash &parent_root) {
    auto child_tries = parent.trieCursor();
    OUTCOME_TRY(child_tries->seekLowerBound(
        common::Buffer::fromString(":child_trie:")));
    using std::string_view_literals::operator""sv;
    while (child_tries->isValid()
           && child_tries->key().value().startsWith(":child_trie:"sv)) {
      auto child_key = child_tries->value().value();
      OUTCOME_TRY(child_hash, trie::RootHash::fromSpan(child_key));
      ref_count_[child_hash]++;
    }
    return outcome::success();
  }

  outcome::result<void> TriePrunerImpl::pruneChildStates(
      const trie::PolkadotTrie &parent) {
    auto child_tries = parent.trieCursor();
    OUTCOME_TRY(child_tries->seekLowerBound(
        common::Buffer::fromString(":child_trie:")));
    using std::string_view_literals::operator""sv;
    while (child_tries->isValid()
           && child_tries->key().value().startsWith(":child_trie:"sv)) {
      auto child_key = child_tries->value().value();
      OUTCOME_TRY(child_hash, trie::RootHash::fromSpan(child_key));
      OUTCOME_TRY(prune(child_hash));
    }
    return outcome::success();
  }

}  // namespace kagome::storage::trie_pruner
