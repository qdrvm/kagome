/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_PROOF_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_PROOF_HPP

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

    auto trie = std::make_shared<storage::trie::PolkadotTrieImpl>();
    for (size_t i = 0; i < chunks.size(); ++i) {
      if (chunks[i].index != i) {
        throw std::logic_error{"ErasureChunk.index is wrong"};
      }
      trie->put(makeTrieProofKey(i), codec.hash256(chunks[i].chunk)).value();
    }

    using Ptr = const storage::trie::TrieNode *;
    std::unordered_map<Ptr, common::Buffer> db;
    auto store = [&](Ptr ptr, common::BufferView, common::Buffer &&encoded) {
      db.emplace(ptr, std::move(encoded));
      return outcome::success();
    };
    auto root_encoded =
        codec
            .encodeNode(
                *trie->getRoot(), storage::trie::StateVersion::V0, store)
            .value();

    for (auto &chunk : chunks) {
      network::ChunkProof proof{root_encoded};
      auto visit = [&](const storage::trie::BranchNode &node, uint8_t index) {
        auto child = dynamic_cast<Ptr>(node.children.at(index).get());
        if (auto it = db.find(child); it != db.end()) {
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
      if (merkle.empty() or merkle == storage::trie::kEmptyRootHash) {
        return nullptr;
      }
      if (codec.isMerkleHash(merkle)) {
        auto it = db.find(common::Hash256::fromSpan(merkle).value());
        if (it == db.end()) {
          return ErasureCodingRootError::MISMATCH;
        }
        merkle = it->second;
      }
      OUTCOME_TRY(n, codec.decodeNode(merkle));
      return std::dynamic_pointer_cast<storage::trie::TrieNode>(n);
    };
    OUTCOME_TRY(root,
                load(std::make_shared<storage::trie::DummyNode>(root_hash)));
    storage::trie::PolkadotTrieImpl trie{root, load};

    OUTCOME_TRY(_expected, trie.get(makeTrieProofKey(chunk.index)));
    OUTCOME_TRY(expected, common::Hash256::fromSpan(_expected));
    auto actual = codec.hash256(chunk.chunk);
    if (actual != expected) {
      return ErasureCodingRootError::MISMATCH;
    }
    return outcome::success();
  }
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_PROOF_HPP
