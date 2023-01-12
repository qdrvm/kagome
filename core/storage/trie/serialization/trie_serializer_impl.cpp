/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/serialization/trie_serializer_impl.hpp"

#include "outcome/outcome.hpp"
#include "storage/trie/codec.hpp"
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

  outcome::result<RootHash> TrieSerializerImpl::storeTrie(PolkadotTrie &trie) {
    if (trie.getRoot() == nullptr) {
      return getEmptyRootHash();
    }
    return storeRootNode(*trie.getRoot());
  }

  outcome::result<std::shared_ptr<PolkadotTrie>>
  TrieSerializerImpl::retrieveTrie(const common::Buffer &db_key) const {
    PolkadotTrie::NodeRetrieveFunctor f =
        [this](const std::shared_ptr<OpaqueTrieNode> &parent)
        -> outcome::result<PolkadotTrie::NodePtr> {
      OUTCOME_TRY(node, retrieveNode(parent));
      return std::move(node);
    };
    if (db_key == getEmptyRootHash()) {
      return trie_factory_->createEmpty(std::move(f));
    }
    OUTCOME_TRY(root, retrieveNode(db_key));
    return trie_factory_->createFromRoot(std::move(root), std::move(f));
  }

  outcome::result<RootHash> TrieSerializerImpl::storeRootNode(TrieNode &node) {
    auto batch = backend_->batch();

    OUTCOME_TRY(
        enc,
        codec_->encodeNodeAndStoreChildren(
            node, [&](common::BufferView hash, common::Buffer &&encoded) {
              return batch->put(hash, std::move(encoded));
            }));
    auto key = codec_->hash256(enc);
    OUTCOME_TRY(batch->put(key, std::move(enc)));
    OUTCOME_TRY(batch->commit());

    return key;
  }

  outcome::result<PolkadotTrie::NodePtr> TrieSerializerImpl::retrieveNode(
      const std::shared_ptr<OpaqueTrieNode> &parent) const {
    if (auto p = std::dynamic_pointer_cast<DummyNode>(parent); p != nullptr) {
      OUTCOME_TRY(n, retrieveNode(p->db_key));
      return std::move(n);
    }
    return std::dynamic_pointer_cast<TrieNode>(parent);
  }

  outcome::result<PolkadotTrie::NodePtr> TrieSerializerImpl::retrieveNode(
      const common::Buffer &db_key) const {
    if (db_key.empty() or db_key == getEmptyRootHash()) {
      return nullptr;
    }
    Buffer enc;
    if (codec_->isMerkleHash(db_key)) {
      OUTCOME_TRY(db, backend_->get(db_key));
      enc = db.into();
    } else {
      // `isMerkleHash(db_key) == false` means `db_key` is value itself
      enc = db_key;
    }
    OUTCOME_TRY(n, codec_->decodeNode(enc));
    return std::dynamic_pointer_cast<TrieNode>(n);
  }

}  // namespace kagome::storage::trie
