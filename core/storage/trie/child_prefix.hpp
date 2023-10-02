/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_CHILD_PREFIX_HPP
#define KAGOME_STORAGE_TRIE_CHILD_PREFIX_HPP

#include "common/buffer_view.hpp"

namespace kagome::storage::trie {
  /**
   * ":child_storage:" prefix matcher
   */
  struct ChildPrefix {
    ChildPrefix();
    ChildPrefix(bool v);

    void match(uint8_t nibble);
    void match(common::BufferView nibbles);

    operator bool() const;

    bool done() const;

   private:
    static constexpr uint8_t kFalse = 0xfe;
    static constexpr uint8_t kTrue = 0xff;
    uint8_t state_ = 0;
  };
}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_CHILD_PREFIX_HPP
