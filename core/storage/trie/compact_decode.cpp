/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/compact_decode.hpp"

#include "common/empty.hpp"
#include "storage/trie/raw_cursor.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, CompactDecodeError, e) {
  using E = decltype(e);
  switch (e) {
    case E::INCOMPLETE_PROOF:
      return "incomplete proof";
    case E::NULL_BRANCH:
      return "Unexpected null branch";
  }
  abort();
}

namespace kagome::storage::trie {
  outcome::result<CompactDecoded> compactDecode(common::BufferView raw_proof) {
    storage::trie::PolkadotCodec codec{};
    OUTCOME_TRY(proof, scale::decode<std::vector<common::Buffer>>(raw_proof));
    CompactDecoded db;
    size_t proof_i = 0;
    while (proof_i < proof.size()) {
      RawCursor<Empty> cursor;
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
        OUTCOME_TRY(cursor.push({
            .node = node,
            .branch = 0,
            .child = false,
            .t = {},
        }));
        return outcome::success();
      };
      OUTCOME_TRY(push());
      while (not cursor.stack.empty()) {
        OUTCOME_TRY(cursor.branchInit());
        while (not cursor.branch_end) {
          if (not cursor.branch_merkle) {
            return CompactDecodeError::NULL_BRANCH;
          }
          if (not cursor.branch_merkle->empty()) {
            OUTCOME_TRY(cursor.branchNext());
            continue;
          }
          OUTCOME_TRY(push());
          break;
        }
        if (cursor.branch_end) {
          auto &node = cursor.stack.back().node;
          OUTCOME_TRY(raw, codec.encodeNode(*node, StateVersion::V0));
          auto hash = codec.hash256(raw);
          db[hash] = {std::move(raw), std::move(node)};
          OUTCOME_TRY(cursor.pop());
          if (not cursor.stack.empty()) {
            *cursor.branch_merkle = hash;
            OUTCOME_TRY(cursor.branchNext());
          }
        }
      }
    }
    return db;
  }
}  // namespace kagome::storage::trie
