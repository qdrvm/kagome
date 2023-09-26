/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/algorithm/string/predicate.hpp>

#include "network/impl/state_sync_request_flow.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/compact_decode.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::network {
  StateSyncRequestFlow::StateSyncRequestFlow(
      std::shared_ptr<storage::trie::TrieStorageBackend> db,
      const primitives::BlockInfo &block_info,
      const primitives::BlockHeader &block)
      : db_{std::move(db)},
        block_info_{block_info},
        block_{block},
        log_{log::createLogger("StateSync")} {
    done_ = isKnown(block.state_root);
    if (not done_) {
      auto &level = levels_.emplace_back();
      level.child = {};
      level.branch_hash = block.state_root;
    }
  }

  bool StateSyncRequestFlow::complete() const {
    return done_;
  }

  StateRequest StateSyncRequestFlow::nextRequest() const {
    BOOST_ASSERT(not complete());
    StateRequest req{block_info_.hash, {}, false};
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
    OUTCOME_TRY(nodes, storage::trie::compactDecode(res.proof));
    auto diff_count = nodes.size(), diff_size = res.proof.size();
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
        level.push(
            {node, std::nullopt, level.child, {it->first, std::move(raw)}});
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
        if (child and not isKnown(*child)) {
          auto &level = levels_.emplace_back();
          level.branch_hash = child;
          pop_level = false;
          break;
        }
        if (level.value_hash and not isKnown(*level.value_hash)) {
          auto it = nodes.find(*level.value_hash);
          if (it == nodes.end()) {
            return outcome::success();
          }
          OUTCOME_TRY(db_->put(it->first, std::move(it->second.first)));
          known_.emplace(it->first);
        }
        for (level.branchInit(); not level.branch_end; level.branchNext()) {
          if (not level.branch_hash or isKnown(*level.branch_hash)) {
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
          OUTCOME_TRY(db_->put(t.hash, std::move(t.encoded)));
          known_.emplace(t.hash);
          level.pop();
          if (not level.stack.empty()) {
            level.branchNext();
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

  bool StateSyncRequestFlow::isKnown(const common::Hash256 &hash) {
    if (hash == storage::trie::kEmptyRootHash) {
      return true;
    }
    if (known_.find(hash) != known_.end()) {
      return true;
    }
    if (auto r = db_->contains(hash); r and r.value()) {
      known_.emplace(hash);
      return true;
    }
    return false;
  }
}  // namespace kagome::network
