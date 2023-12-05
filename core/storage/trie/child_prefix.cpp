/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/child_prefix.hpp"

#include "storage/predefined_keys.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

namespace kagome::storage::trie {
  ChildPrefix::ChildPrefix() : state_{0} {}

  ChildPrefix::ChildPrefix(bool v) : state_{v ? kTrue : kFalse} {}

  void ChildPrefix::match(uint8_t nibble) {
    static const auto kNibbles =
        KeyNibbles::fromByteBuffer(kChildStoragePrefix);
    if (done()) {
      return;
    }
    if (kNibbles[state_] == nibble) {
      ++state_;
      if (state_ == kNibbles.size()) {
        state_ = kTrue;
      }
    } else {
      state_ = kFalse;
    }
  }

  void ChildPrefix::match(common::BufferView nibbles) {
    if (done()) {
      return;
    }
    for (auto &nibble : nibbles) {
      match(nibble);
      if (done()) {
        return;
      }
    }
  }

  ChildPrefix::operator bool() const {
    return state_ == kTrue;
  }

  bool ChildPrefix::done() const {
    return (state_ & kFalse) == kFalse;
  }
}  // namespace kagome::storage::trie
