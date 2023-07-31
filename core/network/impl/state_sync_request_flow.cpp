/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/algorithm/string/predicate.hpp>

#include "network/impl/state_sync_request_flow.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/compact_decode.hpp"
#include "storage/trie/trie_storage_backend.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, StateSyncRequestFlow::Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::EMPTY_RESPONSE:
      return "State sync empty response";
    case E::HASH_MISMATCH:
      return "State sync hash mismatch";
  }
  abort();
}

namespace kagome::network {
  StateSyncRequestFlow::StateSyncRequestFlow(
      std::shared_ptr<storage::trie::TrieStorageBackend> db,
      const primitives::BlockInfo &block_info,
      const primitives::BlockHeader &block)
      : db_{std::move(db)}, block_info_{block_info}, block_{block} {
    if (not isKnown(block.state_root)) {
      levels_.emplace_back(Level{block.state_root, {}});
    }
  }

  bool StateSyncRequestFlow::complete() const {
    return levels_.empty();
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
    BOOST_ASSERT(not complete());
    OUTCOME_TRY(nodes, storage::trie::compactDecode(res.proof));
    while (not levels_.empty()) {
      auto &level = levels_.back();
      auto next_level = false;
      auto on_node = [&](decltype(nodes)::iterator it) {
        storage::trie::ChildPrefix child;
        if (levels_.size() != 1) {
          child = false;
        } else if (not level.stack.empty()) {
          auto &top = level.stack.back();
          child = top.child;
          child.match(*top.branch);
        }
        known_.emplace(it->first);
        auto &item = level.stack.emplace_back(Item{
            it->first,
            std::move(it->second.first),
            std::move(it->second.second),
            std::nullopt,
            child,
        });
        nodes.erase(it);
        item.child.match(item.node->getKeyNibbles());
        if (item.child) {
          auto &value = item.node->getValue().value;
          if (value and value->size() == common::Hash256::size()) {
            auto root = common::Hash256::fromSpan(*value).value();
            levels_.emplace_back(Level{root, {}});
            next_level = true;
          }
        }
      };
      if (level.stack.empty()) {
        auto it = nodes.find(level.root);
        if (it == nodes.end()) {
          return outcome::success();
        }
        on_node(it);
        if (next_level) {
          continue;
        }
      }
      while (not level.stack.empty()) {
        auto &top = level.stack.back();
        if (not top.branch) {
          if (auto &value = top.node->getValue().hash) {
            if (not isKnown(*value)) {
              auto it = nodes.find(*value);
              if (it == nodes.end()) {
                return outcome::success();
              }
              OUTCOME_TRY(db_->put(it->first, std::move(it->second.first)));
              known_.emplace(it->first);
              nodes.erase(it);
            }
          }
          top.branch = 0;
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
            auto hash = dynamic_cast<storage::trie::DummyNode &>(*branch)
                            .db_key.asHash();
            if (not hash or isKnown(*hash)) {
              continue;
            }
            auto it = nodes.find(*hash);
            if (it == nodes.end()) {
              return outcome::success();
            }
            next_stack = true;
            on_node(it);
            break;
          }
          if (next_level) {
            break;
          }
          if (next_stack) {
            continue;
          }
        }
        OUTCOME_TRY(db_->put(top.hash, std::move(top.encoded)));
        level.stack.pop_back();
      }
      if (next_level) {
        continue;
      }
      levels_.pop_back();
    }
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
