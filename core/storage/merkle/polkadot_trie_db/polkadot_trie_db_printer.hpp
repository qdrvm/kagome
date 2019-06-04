/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP_
#define KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP_

#include "storage/merkle/polkadot_trie_db/polkadot_trie_db.hpp"


/**
 * IMPORTANT: This module is meant only for test usage and is not exception-safe
 */

namespace kagome::storage::merkle {

  namespace {
    std::string nibblesToHex(const common::Buffer &key_nibbles) {
      std::stringstream ss;
      for(auto& c: PolkadotCodec::nibblesToKey(key_nibbles)) {
        ss << std::hex << (int)c;
      }
      return ss.str();
    }
  }

  template <typename Stream>
  Stream &operator<<(Stream &s, const PolkadotTrieDb &trie) {
    if(trie.root_.has_value()) {
      auto root = trie.retrieveNode(trie.root_.value()).value();
      printNode(s, root, trie);
    }
    return s;
  }

  template <typename Stream>
  Stream &printNode(Stream &s, PolkadotTrieDb::NodePtr node, const PolkadotTrieDb &trie) {
    using T = PolkadotNode::Type;
    switch (node->getTrieType()) {
      case T::BranchWithValue:
      case T::BranchEmptyValue: {
        auto branch = std::dynamic_pointer_cast<BranchNode>(node);
        s << "(branch) key_nibbles: <" << node->key_nibbles.toHex()
          << "> value: " << node->value.toHex()
          << "\n";
        s << "children: ";
        for (size_t i = 0; i < branch->children.size(); i++) {
          if (branch->children[i]) {
            s << std::hex << i;
          }
        }
        s << "\n";
        for (size_t i = 0; i < branch->children.size(); i++) {
          auto child = branch->children.at(i);
          if (child) {
            if(not child->isDummy()) {
              printNode(s, child, trie);
            } else {
              auto child = trie.retrieveChild(branch, i).value();
              printNode(s, child, trie);
            }
          }
        }
        break;
      }
      case T::Leaf:
        s << "(leaf) key_nibbles: <" << node->key_nibbles.toHex()
          << "> value: " << node->value.toHex()
          << "\n";
        break;

      default:
        s << "(invalid node)\n";
    }
    return s;
  }
}  // namespace kagome::storage::merkle

#endif  // KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP_
