/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/enum_error_code.hpp>
#include "storage/trie/child_prefix.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

namespace kagome::storage::trie {

  enum RawCursorError {
    EmptyStack = 1,
    ChildBranchNotFound,
    StackBackIsNotBranch
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie, RawCursorError);

namespace kagome::storage::trie {

  template <typename T>
  struct RawCursor {
    struct Item {
      std::shared_ptr<TrieNode> node;
      std::optional<uint8_t> branch;
      ChildPrefix child;
      T t;
    };

    outcome::result<void> update() {
      child = false;
      branch_merkle = nullptr;
      branch_hash.reset();
      branch_end = false;
      value_hash = nullptr;
      value_child.reset();
      if (stack.empty()) {
        return outcome::success();
      }
      auto &item = stack.back();
      child = item.child;
      if (item.branch) {
        auto i = *item.branch;
        child.match(i);
        if (item.node->isBranch() and i < BranchNode::kMaxChildren) {
          auto &branches =
              dynamic_cast<storage::trie::BranchNode &>(*item.node).children;
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
          auto &branch = branches[i];
          if (not branch) {
            return RawCursorError::ChildBranchNotFound;
          }
          branch_merkle =
              &dynamic_cast<storage::trie::DummyNode &>(*branch).db_key;
          branch_hash = branch_merkle->asHash();
        } else {
          branch_end = true;
        }
      } else if (auto &value = item.node->getValue()) {
        if (value.hash) {
          value_hash = &*value.hash;
        } else if (item.child and value.value
                   and value.value->size() == common::Hash256::size()) {
          value_child = common::Hash256::fromSpan(*value.value).value();
        }
      }
      return outcome::success();
    }

    outcome::result<void> push(Item &&item) {
      if (not stack.empty() and not stack.back().branch) {
        return RawCursorError::StackBackIsNotBranch;
      }
      item.child.match(item.node->getKeyNibbles());
      stack.emplace_back(std::move(item));
      if (stack.back().branch) {
        OUTCOME_TRY(branchInit());
      } else {
        OUTCOME_TRY(update());
      }
      return outcome::success();
    }

    outcome::result<void> pop() {
      stack.pop_back();
      return update();
    }

    outcome::result<void> branchInit() {
      return branchNext(false);
    }

    outcome::result<void> branchNext(bool next = true) {
      if (stack.empty()) {
        return RawCursorError::EmptyStack;
      }
      auto &item = stack.back();
      auto &i = item.branch;
      if (not i) {
        i = 0;
      }
      if (item.node->isBranch()) {
        auto &branches =
            dynamic_cast<storage::trie::BranchNode &>(*item.node).children;
        for (; *i < BranchNode::kMaxChildren; ++*i) {
          // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
          if (not branches[*i]) {
            continue;
          }
          if (not next) {
            break;
          }
          next = false;
        }
      }
      return update();
    }

    std::vector<Item> stack;

    ChildPrefix child = false;
    MerkleValue *branch_merkle = nullptr;
    std::optional<common::Hash256> branch_hash;
    bool branch_end = false;
    const common::Hash256 *value_hash = nullptr;
    std::optional<common::Hash256> value_child;
  };
}  // namespace kagome::storage::trie
