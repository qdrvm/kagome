/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/serialization/trie_serializer_impl.hpp"

#include "codec.hpp"
#include "outcome/outcome.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {
  TrieSerializerImpl::TrieSerializerImpl(
      std::shared_ptr<PolkadotTrieFactory> factory,
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieStorageBackend> backend)
      : trie_factory_{std::move(factory)},
        codec_{std::move(codec)},
        backend_{std::move(backend)} {
    BOOST_ASSERT(trie_factory_ != nullptr);
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(backend_ != nullptr);
  }

  RootHash TrieSerializerImpl::getEmptyRootHash() const {
    return kEmptyRootHash;
  }

  outcome::result<RootHash> TrieSerializerImpl::storeTrie(
      PolkadotTrie &trie, StateVersion version) {
    current_stats_ = {};
    if (trie.getRoot() == nullptr) {
      return getEmptyRootHash();
    }
    return storeRootNode(*trie.getRoot(), version);
  }

  outcome::result<std::shared_ptr<PolkadotTrie>>
  TrieSerializerImpl::retrieveTrie(const common::Buffer &db_key,
                                   OnNodeLoaded on_node_loaded) const {
    PolkadotTrie::NodeRetrieveFunctor f =
        [this, on_node_loaded](const std::shared_ptr<OpaqueTrieNode> &parent)
        -> outcome::result<PolkadotTrie::NodePtr> {
      OUTCOME_TRY(node, retrieveNode(parent, on_node_loaded));
      return std::move(node);
    };
    if (db_key == getEmptyRootHash()) {
      return trie_factory_->createEmpty(std::move(f));
    }
    OUTCOME_TRY(root, retrieveNode(db_key, on_node_loaded));
    return trie_factory_->createFromRoot(std::move(root), std::move(f));
  }

  outcome::result<RootHash> TrieSerializerImpl::storeRootNode(
      TrieNode &node, StateVersion version) {
    auto batch = backend_->batch();
    BOOST_ASSERT(batch != nullptr);

    OUTCOME_TRY(
        enc,
        codec_->encodeNode(node,
                           version,
                           [&](TrieNode const &node,
                               common::BufferView hash,
                               common::Buffer &&encoded) {
                             current_stats_.new_nodes_written++;
                             if (node.getValue()) {
                               current_stats_.values_written++;
                             }
                             return batch->put(hash, std::move(encoded));
                           }));
    auto hash = codec_->hash256(enc);
    OUTCOME_TRY(batch->put(hash, std::move(enc)));
    OUTCOME_TRY(batch->commit());

    return hash;
  }

  outcome::result<PolkadotTrie::NodePtr> TrieSerializerImpl::retrieveNode(
      const std::shared_ptr<OpaqueTrieNode> &node,
      const OnNodeLoaded &on_node_loaded) const {
    if (auto p = std::dynamic_pointer_cast<DummyValue>(node); p != nullptr) {
      OUTCOME_TRY(value, backend_->get(*p->value.hash));
      if (on_node_loaded) {
        on_node_loaded(value);
      }
      p->value.value = value.into();
      return nullptr;
    }
    if (auto p = std::dynamic_pointer_cast<DummyNode>(node); p != nullptr) {
      OUTCOME_TRY(n, retrieveNode(p->db_key, on_node_loaded));
      return std::move(n);
    }
    return std::dynamic_pointer_cast<TrieNode>(node);
  }

  outcome::result<PolkadotTrie::NodePtr> TrieSerializerImpl::retrieveNode(
      const common::Buffer &db_key, const OnNodeLoaded &on_node_loaded) const {
    if (db_key.empty() or db_key == getEmptyRootHash()) {
      return nullptr;
    }
    Buffer enc;
    if (codec_->isMerkleHash(db_key)) {
      OUTCOME_TRY(db, backend_->get(db_key));
      if (on_node_loaded) {
        on_node_loaded(db);
      }
      enc = db.into();
    } else {
      // `isMerkleHash(db_key) == false` means `db_key` is value itself
      enc = db_key;
    }
    OUTCOME_TRY(n, codec_->decodeNode(enc));
    auto node = std::dynamic_pointer_cast<TrieNode>(n);
    return node;
  }

}  // namespace kagome::storage::trie
