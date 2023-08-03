/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_RAW_CURSOR_HPP
#define KAGOME_STORAGE_TRIE_RAW_CURSOR_HPP

#include "storage/trie/child_prefix.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

namespace kagome::storage::trie {
  template <typename T>
  struct RawCursor {
    struct Item {
      std::shared_ptr<TrieNode> node;
      std::optional<uint8_t> branch;
      ChildPrefix child;
      T t;
    };

    void update() {
      child = false;
      branch_merkle = nullptr;
      branch_hash.reset();
      branch_end = false;
      value_hash = nullptr;
      value_child.reset();
      if (stack.empty()) {
        return;
      }
      auto &item = stack.back();
      child = item.child;
      if (item.branch) {
        auto i = *item.branch;
        child.match(i);
        if (item.node->isBranch() and i < BranchNode::kMaxChildren) {
          auto &branches =
              dynamic_cast<storage::trie::BranchNode &>(*item.node).children;
          auto &branch = branches[i];
          if (not branch) {
            throw std::logic_error{"RawCursor::update branches[branch]=null"};
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
    }

    void push(Item &&item) {
      if (not stack.empty() and not stack.back().branch) {
        throw std::logic_error{"RawCursor::push branch=None"};
      }
      item.child.match(item.node->getKeyNibbles());
      stack.emplace_back(std::move(item));
      if (stack.back().branch) {
        branchInit();
      } else {
        update();
      }
    }

    void pop() {
      stack.pop_back();
      update();
    }

    void branchInit() {
      branchNext(false);
    }

    void branchNext(bool next = true) {
      if (stack.empty()) {
        throw std::logic_error{"RawCursor::branchNext top=null"};
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
          if (not branches[*i]) {
            continue;
          }
          if (not next) {
            break;
          }
          next = false;
        }
      }
      update();
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

#endif  // KAGOME_STORAGE_TRIE_RAW_CURSOR_HPP
