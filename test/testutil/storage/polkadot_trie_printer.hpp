/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <iomanip>

#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"

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

      Stream &printNode(const PolkadotTrie::ConstNodePtr &node,
                        const PolkadotTrie &trie,
                        size_t nest_level = 0) {
        if (node->isBranch()) {
          printBranch(std::static_pointer_cast<const BranchNode>(node),
                      trie,
                      nest_level);
        } else {
          stream_ << std::setfill('-') << std::setw(nest_level) << ""
                  << std::setw(0) << "(leaf) key: <"
                  << hex_lower(node->getKeyNibbles().toByteBuffer())
                  << "> value: " << node->getValue().value->toHex() << "\n";
        }
        return stream_;
      }

     private:
      void printBranch(const PolkadotTrie::ConstBranchPtr &node,
                       const PolkadotTrie &trie,
                       size_t nest_level) {
        std::string indent(nest_level, '\t');
        auto value =
            (node->getValue() ? "\"" + node->getValue().value->toHex() + "\""
                              : "NONE");
        auto branch = std::dynamic_pointer_cast<const BranchNode>(node);
        stream_ << std::setfill('-') << std::setw(nest_level) << ""
                << std::setw(0) << "(branch) key: <"
                << hex_lower(node->getKeyNibbles().toByteBuffer())
                << "> value: " << value << " children: ";
        for (size_t i = 0; i < branch->getChildren().size(); i++) {
          if (branch->getChild(i)) {
            stream_ << std::hex << i << "|";
          }
        }
        stream_ << "\n";
        printEncAndHash(node, nest_level);
        for (size_t i = 0; i < branch->getChildren().size(); i++) {
          auto child = branch->getChild(i);
          if (child) {
            if (auto child_node =
                    std::dynamic_pointer_cast<const TrieNode>(child);
                child_node != nullptr) {
              printNode(child_node, trie, nest_level + 1);
            } else {
              auto fetched_child = trie.retrieveChild(*branch, i).value();
              printNode(fetched_child, trie, nest_level + 1);
            }
          }
        }
      }

      void printEncAndHash(const PolkadotTrie::ConstNodePtr &node,
                           size_t nest_level) {
        auto enc = codec_.encodeNode(*node, {}, {}).value();
        if (print_enc_) {
          stream_ << std::setfill('-') << std::setw(nest_level) << ""
                  << std::setw(0) << "enc: " << enc << "\n";
        }
        if (print_hash_) {
          stream_ << std::setfill('-') << std::setw(nest_level) << ""
                  << std::setw(0) << "hash: "
                  << common::hex_lower(codec_.merkleValue(enc).asBuffer())
                  << "\n";
        }
      }

      Stream &stream_;
      PolkadotCodec codec_;
      bool print_enc_ = false;
      bool print_hash_ = true;
    };
  }  // namespace printer_internal

  template <typename Stream>
  Stream &operator<<(Stream &s, const PolkadotTrie &trie) {
    if (auto root = trie.getRoot()) {
      printer_internal::NodePrinter p(s);
      p.printNode(root, trie);
    }
    return s;
  }

}  // namespace kagome::storage::trie
