/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/assert.hpp>

#include "network/types/collator_messages.hpp"
#include "parachain/availability/erasure_coding_error.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::parachain {
  inline auto makeTrieProofKey(network::ValidatorIndex index) {
    return scale::encode(index).value();
  }

  inline storage::trie::RootHash makeTrieProof(
      std::vector<network::ErasureChunk> &chunks) {
    storage::trie::PolkadotCodec codec;

    auto trie = storage::trie::PolkadotTrieImpl::createEmpty();
    for (size_t i = 0; i < chunks.size(); ++i) {
      if (chunks[i].index != i) {
        throw std::logic_error{"ErasureChunk.index is wrong"};
      }
      trie->put(makeTrieProofKey(i), codec.hash256(chunks[i].chunk)).value();
    }

    using Ptr = const storage::trie::TrieNode *;
    std::unordered_map<Ptr, common::Buffer> db;
    auto store = [&](storage::trie::Codec::Visitee visitee) {
      if (auto child_data =
              std::get_if<storage::trie::Codec::ChildData>(&visitee);
          child_data != nullptr) {
        db.emplace(&child_data->child, std::move(child_data->encoding));
      }
      return outcome::success();
    };
    auto root_encoded =
        codec
            .encodeNode(
                *trie->getRoot(), storage::trie::StateVersion::V0, store)
            .value();

    for (auto &chunk : chunks) {
      network::ChunkProof proof{root_encoded};
      auto visit = [&](const storage::trie::BranchNode &node,
                       uint8_t index,
                       const storage::trie::TrieNode &child) {
        if (auto it = db.find(&child); it != db.end()) {
          proof.emplace_back(it->second);
        }
        return outcome::success();
      };
      trie->forNodeInPath(trie->getRoot(),
                          storage::trie::KeyNibbles::fromByteBuffer(
                              makeTrieProofKey(chunk.index)),
                          visit)
          .value();
      chunk.proof = std::move(proof);
    }

    return codec.hash256(root_encoded);
  }

  inline outcome::result<void> checkTrieProof(
      const network::ErasureChunk &chunk,
      const storage::trie::RootHash &root_hash) {
    storage::trie::PolkadotCodec codec;

    std::unordered_map<common::Hash256, common::Buffer> db;
    for (auto &encoded : chunk.proof) {
      db.emplace(codec.hash256(encoded), encoded);
    }

    // TrieSerializerImpl::retrieveNode
    auto load = [&](const std::shared_ptr<storage::trie::OpaqueTrieNode> &node)
        -> outcome::result<std::shared_ptr<storage::trie::TrieNode>> {
      auto merkle =
          dynamic_cast<const storage::trie::DummyNode &>(*node).db_key;
      // dummy node's key is always a hash
      if (merkle.asHash() == storage::trie::kEmptyRootHash) {
        return nullptr;
      }
      if (merkle.isHash()) {
        auto it = db.find(*merkle.asHash());
        if (it == db.end()) {
          return ErasureCodingRootError::MISMATCH;
        }
        OUTCOME_TRY(n, codec.decodeNode(it->second));
        return std::dynamic_pointer_cast<storage::trie::TrieNode>(n);
      }
      OUTCOME_TRY(n, codec.decodeNode(merkle.asBuffer()));
      return std::dynamic_pointer_cast<storage::trie::TrieNode>(n);
    };
    auto load_value = [](const common::Hash256 &hash)
        -> outcome::result<std::optional<common::Buffer>> {
      BOOST_ASSERT_MSG(false, "Hashed values should not appear in proofs");
      return std::nullopt;
    };
    OUTCOME_TRY(root,
                load(std::make_shared<storage::trie::DummyNode>(root_hash)));
    auto trie =
        storage::trie::PolkadotTrieImpl::create(root, {load, load_value});

    OUTCOME_TRY(_expected, trie->get(makeTrieProofKey(chunk.index)));
    OUTCOME_TRY(expected, common::Hash256::fromSpan(_expected));
    auto actual = codec.hash256(chunk.chunk);
    if (actual != expected) {
      return ErasureCodingRootError::MISMATCH;
    }
    return outcome::success();
  }
}  // namespace kagome::parachain
