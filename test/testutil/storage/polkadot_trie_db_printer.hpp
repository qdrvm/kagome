/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP

#include "storage/trie/polkadot_trie_db/polkadot_trie_db.hpp"

/**
 * IMPORTANT: This module is meant only for test usage and is not exception-safe
 */

namespace kagome::storage::trie {

  namespace {
    std::string nibblesToStr(const Buffer &nibbles) {
      std::stringstream s;
      for (auto nibble : nibbles) {
        if (nibble < 10)
          s << static_cast<char>('0' + nibble);
        else
          s << static_cast<char>('a' + (nibble - 10));
      }
      return s.str();
    }
  }  // namespace

  template <typename Stream>
  Stream &operator<<(Stream &s, const PolkadotTrieDb &trie) {
    if (trie.root_.has_value()) {
      auto root = trie.retrieveNode(trie.root_.value()).value();
      printNode(s, root, trie, 0);
    }
    return s;
  }

  template <typename Stream>
  Stream &printNode(Stream &s, PolkadotTrieDb::NodePtr node,
                    const PolkadotTrieDb &trie, size_t nest_level) {
    using T = PolkadotNode::Type;
    std::string indent(nest_level, '\t');
    switch (node->getTrieType()) {
      case T::BranchWithValue:
      case T::BranchEmptyValue: {
        auto branch = std::dynamic_pointer_cast<BranchNode>(node);
        s << indent << "(branch) key_nibbles: <"
          << nibblesToStr(node->key_nibbles)
          << "> value: " << node->value.toHex() << " children: ";
        for (size_t i = 0; i < branch->children.size(); i++) {
          if (branch->children[i]) {
            s << std::hex << i;
          }
        }
        s << "\n";
        for (size_t i = 0; i < branch->children.size(); i++) {
          auto child = branch->children.at(i);
          if (child) {
            if (not child->isDummy()) {
              printNode(s, child, trie, nest_level + 1);
            } else {
              auto fetched_child = trie.retrieveChild(branch, i).value();
              printNode(s, fetched_child, trie, nest_level + 1);
            }
          }
        }
        break;
      }
      case T::Leaf:
        s << indent << "(leaf) key_nibbles: <"
          << nibblesToStr(node->key_nibbles)
          << "> value: " << node->value.toHex() << "\n";
        break;

      default:
        s << "(invalid node)\n";
    }
    return s;
  }
}  // namespace kagome::storage::trie

#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP
