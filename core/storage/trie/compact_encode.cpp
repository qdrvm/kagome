/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/compact_encode.hpp"

#include <qtils/enum_error_code.hpp>
#include <unordered_set>

#include "storage/trie/raw_cursor.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, RawCursorError, e) {
  using E = kagome::storage::trie::RawCursorError;
  switch (e) {
    case E::EmptyStack:
      return "Unexpected empty stack";
    case E::ChildBranchNotFound:
      return "Expected child branch is not found";
    case E::StackBackIsNotBranch:
      return "No branch at the end of the stack";
  }
  return "Unknown RawCursorError";
}

namespace kagome::storage::trie {
  outcome::result<common::Buffer> compactEncode(const OnRead &db,
                                                const common::Hash256 &root) {
    storage::trie::PolkadotCodec codec;
    std::vector<RawCursor<size_t>> levels;
    auto &level = levels.emplace_back();
    level.child = {};
    std::unordered_set<common::Hash256> seen, value_seen;
    std::vector<std::vector<common::Buffer>> proofs(2);
    auto push = [&](const common::Hash256 &hash) -> outcome::result<bool> {
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
      const auto &value = node->getValue();
      if (value.hash) {
        auto it = db.db.find(*value.hash);
        if (it != db.db.end() and value_seen.emplace(*value.hash).second) {
          compact = it->second;
          node->setValueHash(std::nullopt);
          node->setValue(common::Buffer{});
        }
      }
      auto &level = levels.back();
      auto &proof = proofs[levels.size() - 1];
      OUTCOME_TRY(level.push({
          .node = std::move(node),
          .child = level.child,
          .t = proof.size(),
      }));
      proof.emplace_back();
      if (compact) {
        proof.back().putUint8(kEscapeCompactHeader);
        proof.emplace_back(*compact);
      }
      return true;
    };
    OUTCOME_TRY(push(root));
    while (not levels.empty()) {
      auto &level = levels.back();
      auto pop_level = true;
      while (not level.stack.empty()) {
        auto child = level.value_child;
        if (child and seen.emplace(*child).second) {
          levels.emplace_back();
          OUTCOME_TRY(push(*child));
          pop_level = false;
          break;
        }
        OUTCOME_TRY(level.branchInit());
        while (not level.branch_end) {
          OUTCOME_TRY(push_success, push(*level.branch_hash));
          if (level.branch_hash and seen.emplace(*level.branch_hash).second
              and push_success) {
            break;
          }
          OUTCOME_TRY(level.branchNext());
        }
        if (level.branch_end) {
          auto &item = level.stack.back();
          auto &proof = proofs[levels.size() - 1][item.t];
          proof.put(codec.encodeNode(*item.node, StateVersion::V0).value());
          OUTCOME_TRY(level.pop());
          if (not level.stack.empty()) {
            *level.branch_merkle = MerkleValue::create({}).value();
            OUTCOME_TRY(level.branchNext());
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
