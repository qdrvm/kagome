/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP_
#define KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP_

#include "storage/merkle/polkadot_trie_db/polkadot_trie_db.hpp"

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
    s << *trie.root_;
    return s;
  }

  template <typename Stream>
  Stream &operator<<(Stream &s, const PolkadotNode &node) {
    using T = PolkadotNode::Type;
    switch (node.getTrieType()) {
      case T::BranchWithValue:
      case T::BranchEmptyValue: {
        auto &branch = dynamic_cast<BranchNode const &>(node);
        s << "(branch) key_nibbles: <" << nibblesToHex(node.key_nibbles)
          << "> value: " << node.value.toHex()
          << "\n";
        s << "children: ";
        for (size_t i = 0; i < branch.children.size(); i++) {
          if (branch.children[i]) {
            s << std::hex << i;
          }
        }
        s << "\n";
        for (auto &child : branch.children) {
          if (child) {
            s << *child;
          }
        }
        break;
      }
      case T::Leaf:
        s << "(leaf) key_nibbles: <" << nibblesToHex(node.key_nibbles)
          << "> value: " << node.value.toHex()
          << "\n";
        break;

      default:
        s << "(invalid node)\n";
    }
    return s;
  }
}  // namespace kagome::storage::merkle

#endif  // KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP_
