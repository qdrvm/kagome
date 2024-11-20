/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/algorithm/string/predicate.hpp>

#include "crypto/blake2/blake2b.h"
#include "network/impl/state_sync_request_flow.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/compact_decode.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::network {

  outcome::result<std::unique_ptr<StateSyncRequestFlow>>
  StateSyncRequestFlow::create(
      std::shared_ptr<storage::trie::TrieStorageBackend> node_db,
      const primitives::BlockInfo &block_info,
      const primitives::BlockHeader &block) {
    std::unique_ptr<StateSyncRequestFlow> flow{
        new StateSyncRequestFlow(node_db, block_info, block)};
    OUTCOME_TRY(done, flow->isKnown(block.state_root));
    flow->done_ = done;
    if (not done) {
      flow->levels_.emplace_back(Level{.branch_hash = block.state_root});
    }
    return flow;
  }

  StateSyncRequestFlow::StateSyncRequestFlow(
      std::shared_ptr<storage::trie::TrieStorageBackend> node_db,
      const primitives::BlockInfo &block_info,
      const primitives::BlockHeader &block)
      : node_db_{std::move(node_db)},
        block_info_{block_info},
        block_{block},
        log_{log::createLogger("StateSync")} {}

  StateRequest StateSyncRequestFlow::nextRequest() const {
    BOOST_ASSERT(not complete());
    StateRequest req{
        .hash = block_info_.hash,
        .start = {},
        .no_proof = false,
    };
    for (auto &level : levels_) {
      storage::trie::KeyNibbles nibbles;
      for (auto &item : level.stack) {
        nibbles.put(item.node->getKeyNibbles());
        if (item.branch) {
          nibbles.putUint8(*item.branch);
        }
      }
      if (nibbles.size() % 2 != 0) {
        nibbles.putUint8(0);
      }
      req.start.emplace_back(nibbles.toByteBuffer());
    }
    return req;
  }

  outcome::result<void> StateSyncRequestFlow::onResponse(
      const StateResponse &res) {
    storage::trie::PolkadotCodec codec;
    BOOST_ASSERT(not complete());
    BOOST_OUTCOME_TRY(auto nodes, storage::trie::compactDecode(res.proof));
    auto diff_count = nodes.size();
    auto diff_size = res.proof.size();
    if (diff_count != 0) {
      stat_count_ += diff_count;
      stat_size_ += diff_size;
      SL_INFO(log_,
              "received {} nodes {}mb, total {} nodes {}mb",
              diff_count,
              diff_size >> 20,
              stat_count_,
              stat_size_ >> 20);
    }
    while (not levels_.empty()) {
      auto &level = levels_.back();
      auto push = [&](decltype(nodes)::iterator it) -> outcome::result<void> {
        auto &node = it->second.second;
        auto &raw = it->second.first;
        if (not node) {
          // when trie node is contained in other node value
          BOOST_OUTCOME_TRY(node, codec.decodeNode(raw));
        }
        OUTCOME_TRY(level.push({
            .node = node,
            .branch = std::nullopt,
            .child = level.child,
            .t =
                {
                    .hash = it->first,
                    .encoded = std::move(raw),
                },
        }));
        nodes.erase(it);
        return outcome::success();
      };
      if (level.stack.empty()) {
        auto it = nodes.find(*level.branch_hash);
        if (it == nodes.end()) {
          return outcome::success();
        }
        OUTCOME_TRY(push(it));
      }
      auto pop_level = true;
      while (not level.stack.empty()) {
        auto child = level.value_child;
        OUTCOME_TRY(known, isKnown(*child));
        if (child and not known) {
          auto &level = levels_.emplace_back();
          level.branch_hash = child;
          pop_level = false;
          break;
        }
        if (level.value_hash) {
          OUTCOME_TRY(known_value, isKnown(*level.value_hash));
          if (not known_value) {
            auto it = nodes.find(*level.value_hash);
            if (it == nodes.end()) {
              return outcome::success();
            }
            OUTCOME_TRY(
                node_db_->values().put(it->first, std::move(it->second.first)));
            known_.emplace(it->first);
          }
        }
        OUTCOME_TRY(level.branchInit());
        while (not level.branch_end) {
          OUTCOME_TRY(known, isKnown(*level.branch_hash));
          if (not level.branch_hash or known) {
            OUTCOME_TRY(level.branchNext());
            continue;
          }
          auto it = nodes.find(*level.branch_hash);
          if (it == nodes.end()) {
            return outcome::success();
          }
          OUTCOME_TRY(push(it));
          break;
        }
        if (level.branch_end) {
          auto &t = level.stack.back().t;
          OUTCOME_TRY(node_db_->nodes().put(t.hash, std::move(t.encoded)));
          known_.emplace(t.hash);
          OUTCOME_TRY(level.pop());
          if (not level.stack.empty()) {
            OUTCOME_TRY(level.branchNext());
          }
        }
      }
      if (pop_level) {
        levels_.pop_back();
      }
    }
    done_ = true;
    return outcome::success();
  }

  outcome::result<bool> StateSyncRequestFlow::isKnown(
      const common::Hash256 &hash) {
    if (hash == storage::trie::kEmptyRootHash) {
      return true;
    }
    if (known_.find(hash) != known_.end()) {
      return true;
    }
    OUTCOME_TRY(known_node, node_db_->nodes().contains(hash));
    OUTCOME_TRY(known_value, node_db_->values().contains(hash));
    if (known_node || known_value) {
      known_.emplace(hash);
      return true;
    }
    return false;
  }
}  // namespace kagome::network
