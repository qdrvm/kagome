/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/compact_encode.hpp"

#include <unordered_set>

#include "storage/trie/raw_cursor.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::storage::trie {
  outcome::result<common::Buffer> compactEncode(const OnRead &db,
                                                const common::Hash256 &root) {
    storage::trie::PolkadotCodec codec;
    std::vector<RawCursor<size_t>> levels;
    auto &level = levels.emplace_back();
    level.child = {};
    std::unordered_set<common::Hash256> seen, value_seen;
    std::vector<std::vector<common::Buffer>> proofs(2);
    auto push = [&](const common::Hash256 &hash) {
      auto it = db.db.find(hash);
      if (it == db.db.end()) {
        return false;
      }
      auto node_res = codec.decodeNode(it->second);
      if (not node_res) {
        return false;
      }
      auto &node = node_res.value();
      std::optional<common::BufferView> compact;
      auto &value = node->getMutableValue();
      if (value.hash) {
        auto it = db.db.find(*value.hash);
        if (it != db.db.end() and value_seen.emplace(*value.hash).second) {
          compact = it->second;
          value.hash.reset();
          value.value.emplace();
        }
      }
      auto &level = levels.back();
      auto &proof = proofs[levels.size() - 1];
      level.push({std::move(node), std::nullopt, level.child, proof.size()});
      proof.emplace_back();
      if (compact) {
        proof.back().putUint8(kEscapeCompactHeader);
        proof.emplace_back(*compact);
      }
      return true;
    };
    push(root);
    while (not levels.empty()) {
      auto &level = levels.back();
      auto pop_level = true;
      while (not level.stack.empty()) {
        auto child = level.value_child;
        if (child and seen.emplace(*child).second) {
          levels.emplace_back();
          push(*child);
          pop_level = false;
          break;
        }
        for (level.branchInit(); not level.branch_end; level.branchNext()) {
          if (level.branch_hash and seen.emplace(*level.branch_hash).second
              and push(*level.branch_hash)) {
            break;
          }
        }
        if (level.branch_end) {
          auto &item = level.stack.back();
          auto &proof = proofs[levels.size() - 1][item.t];
          proof.put(codec.encodeNode(*item.node, StateVersion::V0).value());
          level.pop();
          if (not level.stack.empty()) {
            *level.branch_merkle = MerkleValue::create({}).value();
            level.branchNext();
          }
        }
      }
      if (pop_level) {
        levels.pop_back();
      }
    }

    // encode concatenated vectors
    scale::ScaleEncoderStream s;
    try {
      s << scale::CompactInteger{proofs[0].size() + proofs[1].size()};
      for (auto &proof : proofs) {
        for (auto &x : proof) {
          s << x;
        }
      }
    } catch (std::system_error &e) {
      return e.code();
    }
    return common::Buffer{s.to_vector()};
  }
}  // namespace kagome::storage::trie
