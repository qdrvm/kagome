/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP

#include "storage/trie/impl/polkadot_trie_db.hpp"

/**
 * IMPORTANT: This module is meant only for test usage and is not exception-safe
 */

namespace kagome::storage::trie {

  inline std::string nibblesToStr(const kagome::common::Buffer &nibbles) {
    std::stringstream s;
    for (auto nibble : nibbles) {
      if (nibble < 10) {
        s << static_cast<char>('0' + nibble);
      } else {
        s << static_cast<char>('a' + (nibble - 10));
      }
    }
    return s.str();
  }  // namespace kagome::storage::trie

  template <typename Stream>
  Stream &operator<<(Stream &s, const PolkadotTrieDb &trie) {
    if (not trie.empty()) {
      auto root = trie.retrieveNode(trie.getRootHash()).value();
      printNode(s, root, trie, 0);
    }
    return s;
  }

  template <typename Stream>
  Stream &printNode(Stream &s, const PolkadotTrieDb::NodePtr &node,
                    const PolkadotTrieDb &trie, size_t nest_level) {
    using T = PolkadotNode::Type;
    std::string indent(nest_level, '\t');
    switch (node->getTrieType()) {
      case T::BranchWithValue:
      case T::BranchEmptyValue: {
        auto branch = std::dynamic_pointer_cast<BranchNode>(node);
        s << indent << "(branch) key: <"
          << hex_lower(trie.codec_.nibblesToKey(node->key_nibbles))
          << "> value: " << (node->value ? "\"" + node->value.get().toHex() + "\"" : "No value") << " children: ";
        for (size_t i = 0; i < branch->children.size(); i++) {
          if (branch->children[i]) {
            s << std::hex << i << "|";
          }
        }
        s << "\n";
        auto enc = trie.codec_.encodeNode(*node).value();
        s << indent << "enc: " << enc << "\n";
        s << indent << "hash: " << common::hex_upper(trie.codec_.merkleValue(enc)) << "\n";
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
      case T::Leaf: {
        s << indent << "(leaf) key: <"
          << hex_lower(trie.codec_.nibblesToKey(node->key_nibbles))
          << "> value: " << node->value.get().toHex() << "\n";
        auto enc = trie.codec_.encodeNode(*node).value();
        s << indent << "enc: " << enc << "\n";
        s << indent << "hash: " << common::hex_upper(trie.codec_.merkleValue(enc)) << "\n";
        break;
      }
      default:
        s << "(invalid node)\n";
    }
    return s;
  }
}  // namespace kagome::storage::trie

#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP
