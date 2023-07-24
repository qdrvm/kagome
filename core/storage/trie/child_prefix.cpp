/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
    if ((state_ & kFalse) == kFalse) {
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
    if ((state_ & kFalse) == kFalse) {
      return;
    }
    for (auto &nibble : nibbles) {
      match(nibble);
      if ((state_ & kFalse) == kFalse) {
        break;
      }
    }
  }

  ChildPrefix::operator bool() const {
    return state_ == kTrue;
  }
}  // namespace kagome::storage::trie
