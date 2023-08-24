/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/serialization/trie_serializer_impl.hpp"

#include "common/monadic_utils.hpp"
#include "outcome/outcome.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"
#include "storage/trie/serialization/codec.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::trie {
  TrieSerializerImpl::TrieSerializerImpl(
      std::shared_ptr<PolkadotTrieFactory> factory,
      std::shared_ptr<Codec> codec,
      std::shared_ptr<TrieNodeStorageBackend> node_backend,
      std::shared_ptr<TrieValueStorageBackend> value_backend)
      : trie_factory_{std::move(factory)},
        codec_{std::move(codec)},
        node_backend_{std::move(node_backend)},
        value_backend_{std::move(value_backend)}{
    BOOST_ASSERT(trie_factory_ != nullptr);
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(node_backend_ != nullptr);
    BOOST_ASSERT(value_backend_ != nullptr);
  }

  RootHash TrieSerializerImpl::getEmptyRootHash() const {
    return kEmptyRootHash;
  }

  outcome::result<RootHash> TrieSerializerImpl::storeTrie(
      PolkadotTrie &trie, StateVersion version) {
    if (trie.getRoot() == nullptr) {
      return getEmptyRootHash();
    }
    return storeRootNode(*trie.getRoot(), version);
  }

  outcome::result<std::shared_ptr<PolkadotTrie>>
  TrieSerializerImpl::retrieveTrie(RootHash db_key,
                                   OnNodeLoaded on_node_loaded) const {
    PolkadotTrie::NodeRetrieveFunction f =
        [this, on_node_loaded](const std::shared_ptr<OpaqueTrieNode> &parent)
        -> outcome::result<PolkadotTrie::NodePtr> {
      OUTCOME_TRY(node, retrieveNode(parent, on_node_loaded));
      return std::move(node);
    };
    PolkadotTrie::ValueRetrieveFunction v =
        [this, on_node_loaded](const common::Hash256 &hash)
        -> outcome::result<std::optional<common::Buffer>> {
      OUTCOME_TRY(value, retrieveValue(hash, on_node_loaded));
      return value;
    };
    if (db_key == getEmptyRootHash()) {
      return trie_factory_->createEmpty(
          PolkadotTrie::RetrieveFunctions{std::move(f), std::move(v)});
    }
    OUTCOME_TRY(root, retrieveNode(db_key, on_node_loaded));
    return trie_factory_->createFromRoot(
        std::move(root),
        PolkadotTrie::RetrieveFunctions{std::move(f), std::move(v)});
  }

  outcome::result<RootHash> TrieSerializerImpl::storeRootNode(
      TrieNode &node, StateVersion version) {
    auto batch = node_backend_->batch();
    BOOST_ASSERT(batch != nullptr);

    OUTCOME_TRY(
        enc,
        codec_->encodeNode(
            node,
            version,
            [&](Codec::Visitee visitee) -> outcome::result<void> {
              if (auto child_data = std::get_if<Codec::ChildData>(&visitee);
                  child_data != nullptr) {
                if (child_data->merkle_value.isHash()) {
                  return batch->put(child_data->merkle_value.asBuffer(),
                                    std::move(child_data->encoding));
                } else {
                  return outcome::success();  // nodes which encoding is shorter
                                              // than its hash are not stored in
                                              // the DB separately
                }
              }
              auto value_data = std::get<Codec::ValueData>(visitee);
              // value_data.value is a reference to a buffer stored outside of
              // this lambda, so taking its view should be okay
              return batch->put(value_data.hash, value_data.value.view());
            }));
    auto hash = codec_->hash256(enc);
    OUTCOME_TRY(batch->put(hash, std::move(enc)));
    OUTCOME_TRY(batch->commit());

    return hash;
  }

  outcome::result<PolkadotTrie::NodePtr> TrieSerializerImpl::retrieveNode(
      const std::shared_ptr<OpaqueTrieNode> &node,
      const OnNodeLoaded &on_node_loaded) const {
    if (auto p = std::dynamic_pointer_cast<DummyNode>(node); p != nullptr) {
      OUTCOME_TRY(n, retrieveNode(p->db_key, on_node_loaded));
      return std::move(n);
    }
    return std::dynamic_pointer_cast<TrieNode>(node);
  }

  outcome::result<PolkadotTrie::NodePtr> TrieSerializerImpl::retrieveNode(
      MerkleValue db_key, const OnNodeLoaded &on_node_loaded) const {
    if (db_key.asHash() == getEmptyRootHash()) {
      return nullptr;
    }
    Buffer enc;
    if (db_key.isHash()) {
      OUTCOME_TRY(db, node_backend_->get(db_key.asBuffer()));
      if (on_node_loaded) {
        on_node_loaded(db);
      }
      enc = db.intoBuffer();
    } else {
      // `isMerkleHash(db_key) == false` means `db_key` is value itself
      enc = db_key.asBuffer();
    }
    OUTCOME_TRY(n, codec_->decodeNode(enc));
    auto node = std::dynamic_pointer_cast<TrieNode>(n);
    return node;
  }

  outcome::result<std::optional<common::Buffer>>
  TrieSerializerImpl::retrieveValue(const common::Hash256 &hash,
                                    const OnNodeLoaded &on_node_loaded) const {
    OUTCOME_TRY(value, value_backend_->tryGet(hash));
    return common::map_optional(std::move(value),
                                [&](common::BufferOrView &&value) {
                                  if (on_node_loaded) {
                                    on_node_loaded(value);
                                  }
                                  return value.intoBuffer();
                                });
  }

}  // namespace kagome::storage::trie
