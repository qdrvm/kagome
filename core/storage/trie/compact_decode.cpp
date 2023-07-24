/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/compact_decode.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, CompactDecodeError, e) {
  using E = decltype(e);
  switch (e) {
    case E::INCOMPLETE_PROOF:
      return "incomplete proof";
  }
  abort();
}

namespace kagome::storage::trie {
  outcome::result<CompactDecoded> compactDecode(common::BufferView raw_proof) {
    storage::trie::PolkadotCodec codec;
    OUTCOME_TRY(proof, scale::decode<std::vector<common::Buffer>>(raw_proof));
    CompactDecoded db;
    std::vector<std::pair<std::shared_ptr<TrieNode>, uint8_t>> stack;
    size_t proof_i = 0;
    auto push = [&]() mutable -> outcome::result<void> {
      if (proof_i >= proof.size()) {
        return CompactDecodeError::INCOMPLETE_PROOF;
      }
      common::BufferView raw{proof[proof_i]};
      ++proof_i;
      auto compact = not raw.empty() and raw[0] == kEscapeCompactHeader;
      if (compact) {
        raw = raw.subspan(1);
        if (proof_i >= proof.size()) {
          return CompactDecodeError::INCOMPLETE_PROOF;
        }
      }
      OUTCOME_TRY(node, codec.decodeNode(raw));
      if (compact) {
        auto &value = proof[proof_i];
        ++proof_i;
        auto hash = codec.hash256(value);
        db.emplace(hash, std::make_pair(std::move(value), nullptr));
        node->setValue({std::nullopt, hash});
      }
      stack.emplace_back(node, 0);
      return outcome::success();
    };
    while (proof_i < proof.size()) {
      OUTCOME_TRY(push());
      std::optional<common::Hash256> hash;
      while (not stack.empty()) {
        auto &[node, i] = stack.back();
        auto pop = true;
        if (node->isBranch()) {
          auto &branches =
              dynamic_cast<storage::trie::BranchNode &>(*node).children;
          for (; i < branches.size(); ++i) {
            auto &branch = branches[i];
            if (not branch) {
              continue;
            }
            auto &merkle =
                dynamic_cast<storage::trie::DummyNode &>(*branch).db_key;
            if (not merkle.empty()) {
              continue;
            }
            if (hash) {
              merkle = *hash;
              hash.reset();
              continue;
            }
            pop = false;
            OUTCOME_TRY(push());
            break;
          }
        }
        if (not pop) {
          continue;
        }
        OUTCOME_TRY(raw, codec.encodeNode(*node, StateVersion::V0));
        hash = codec.hash256(raw);
        db[*hash] = {std::move(raw), std::move(node)};
        stack.pop_back();
      }
    }
    return db;
  }
}  // namespace kagome::storage::trie
