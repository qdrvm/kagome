/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/compact_encode.hpp"

#include <unordered_set>

#include "storage/trie/child_prefix.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

namespace kagome::storage::trie {
  outcome::result<common::Buffer> compactEncode(const OnRead &db,
                                                const common::Hash256 &root) {
    storage::trie::PolkadotCodec codec;
    struct Item {
      size_t proof_i;
      std::shared_ptr<storage::trie::TrieNode> node;
      std::optional<uint8_t> branch;
      storage::trie::ChildPrefix child;
      bool compact;
    };
    std::unordered_set<common::Hash256> seen, value_seen;
    std::vector<std::vector<common::Buffer>> proofs(2);
    std::vector<std::vector<Item>> levels(1);
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
      if (value.hash and value_seen.emplace(*value.hash).second) {
        auto it = db.db.find(*value.hash);
        if (it != db.db.end()) {
          compact = it->second;
          value.hash.reset();
          value.value.emplace();
        }
      }
      auto &stack = levels.back();
      auto &proof = proofs[levels.size() - 1];
      storage::trie::ChildPrefix child;
      if (levels.size() != 1) {
        child = false;
      } else if (not stack.empty()) {
        auto &top = stack.back();
        child = top.child;
        child.match(*top.branch);
      }
      child.match(node->getKeyNibbles());
      stack.emplace_back(Item{
          proof.size(),
          std::move(node),
          std::nullopt,
          child,
          compact.has_value(),
      });
      proof.emplace_back();
      if (compact) {
        proof.emplace_back(*compact);
      }
      return true;
    };
    push(root);
    while (not levels.empty()) {
      auto next_level = false;
      auto &stack = levels.back();
      while (not stack.empty()) {
        auto &top = stack.back();
        auto &value = top.node->getMutableValue();
        if (not top.branch) {
          top.branch = 0;
          if (top.child and value.value
              and value.value->size() == common::Hash256::size()) {
            auto root = common::Hash256::fromSpan(*value.value).value();
            if (seen.emplace(root).second) {
              levels.emplace_back();
              push(root);
              next_level = true;
              break;
            }
          }
        }
        if (top.node->isBranch()) {
          auto &branches =
              dynamic_cast<storage::trie::BranchNode &>(*top.node).children;
          auto next_stack = false;
          for (; *top.branch < branches.size(); ++*top.branch) {
            auto &branch = branches[*top.branch];
            if (not branch) {
              continue;
            }
            auto &merkle =
                dynamic_cast<storage::trie::DummyNode &>(*branch).db_key;
            auto hash = merkle.asHash();
            if (not hash) {
              continue;
            }
            if (not seen.emplace(*hash).second) {
              continue;
            }
            if (push(*hash)) {
              merkle = MerkleValue::create({}).value();
              next_stack = true;
              break;
            }
          }
          if (next_level) {
            break;
          }
          if (next_stack) {
            continue;
          }
        }
        auto &proof = proofs[levels.size() - 1][top.proof_i];
        if (top.compact) {
          proof.putUint8(kEscapeCompactHeader);
        }
        proof.put(codec.encodeNode(*top.node, StateVersion::V0).value());
        stack.pop_back();
      }
      if (next_level) {
        continue;
      }
      levels.pop_back();
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
