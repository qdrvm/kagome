/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP

#include "storage/trie/polkadot_trie/polkadot_trie.hpp"

/**
 * IMPORTANT: This module is meant only for test usage and is not exception-safe
 */

namespace kagome::storage::trie {

  namespace printer_internal {
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
    }

    template <typename Stream>
    class NodePrinter {
     public:
      explicit NodePrinter(Stream &s) : stream_{s} {}

      Stream &printNode(const PolkadotTrie::NodePtr &node,
                        const PolkadotTrie &trie,
                        size_t nest_level = 0) {
        using T = PolkadotNode::Type;
        switch (node->getTrieType()) {
          case T::BranchWithValue:
          case T::BranchEmptyValue: {
            printBranch(std::static_pointer_cast<BranchNode>(node), trie, nest_level);
            break;
          }
          case T::Leaf: {
            stream_ << std::setfill('-') << std::setw(nest_level) << ""
                    << std::setw(0) << "(leaf) key: <"
                    << hex_lower(codec_.nibblesToKey(node->key_nibbles))
                    << "> value: " << node->value.get().toHex() << "\n";
            break;
          }
          default:
            stream_ << "(invalid node)\n";
        }
        return stream_;
      }

     private:
      void printBranch(const PolkadotTrie::BranchPtr &node,
                       const PolkadotTrie &trie,
                       size_t nest_level) {
        std::string indent(nest_level, '\t');
        auto value =
            (node->value ? "\"" + node->value.get().toHex() + "\"" : "NONE");
        auto branch = std::dynamic_pointer_cast<BranchNode>(node);
        stream_ << std::setfill('-') << std::setw(nest_level) << ""
                << std::setw(0) << "(branch) key: <"
                << hex_lower(codec_.nibblesToKey(node->key_nibbles))
                << "> value: " << value << " children: ";
        for (size_t i = 0; i < branch->children.size(); i++) {
          if (branch->children[i]) {
            stream_ << std::hex << i << "|";
          }
        }
        stream_ << "\n";
        printEncAndHash(node, nest_level);
        for (size_t i = 0; i < branch->children.size(); i++) {
          auto child = branch->children.at(i);
          if (child) {
            if (not child->isDummy()) {
              printNode(child, trie, nest_level + 1);
            } else {
              auto fetched_child = trie.retrieveChild(branch, i).value();
              printNode(fetched_child, trie, nest_level + 1);
            }
          }
        }
      }

      void printEncAndHash(const PolkadotTrie::NodePtr &node,
                           size_t nest_level) {
        auto enc = codec_.encodeNode(*node).value();
        if (print_enc_) {
          stream_ << std::setfill('-') << std::setw(nest_level) << ""
                  << std::setw(0) << "enc: " << enc << "\n";
        }
        if (print_hash_) {
          stream_ << std::setfill('-') << std::setw(nest_level) << ""
                  << std::setw(0)
                  << "hash: " << common::hex_upper(codec_.merkleValue(enc))
                  << "\n";
        }
      }

      Stream &stream_;
      PolkadotCodec codec_;
      bool print_enc_;
      bool print_hash_;
    };
  }  // namespace printer_internal

  template <typename Stream>
  Stream &operator<<(Stream &s, const PolkadotTrie &trie) {
    if (not trie.empty()) {
      auto root = trie.getRoot();
      printer_internal::NodePrinter p(s);
      p.printNode(root, trie);
    }
    return s;
  }

}  // namespace kagome::storage::trie

#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_POLKADOT_TRIE_DB_PRINTER_HPP
