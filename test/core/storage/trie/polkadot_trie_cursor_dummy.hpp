/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_STORAGE_TRIE_POLKADOT_TRIE_CURSOR_DUMMY_HPP
#define KAGOME_TEST_CORE_STORAGE_TRIE_POLKADOT_TRIE_CURSOR_DUMMY_HPP

#include "storage/trie/polkadot_trie/polkadot_trie_cursor.hpp"

namespace kagome::storage::trie {

  // Dummy implementation of cursor that wraps std::map
  class PolkadotTrieCursorDummy : public PolkadotTrieCursor {
   private:
    std::map<common::Buffer, common::Buffer, std::less<>> key_val_;
    decltype(key_val_.begin()) current_;

   public:
    explicit PolkadotTrieCursorDummy(
        const std::map<common::Buffer, common::Buffer, std::less<>> &key_val)
        : key_val_{key_val} {}

    outcome::result<bool> seekFirst() override {
      current_ = key_val_.begin();
      return true;
    }

    outcome::result<bool> seek(const common::BufferView &key) override {
      current_ = key_val_.find(key);
      return current_ != key_val_.end();
    }

    outcome::result<void> seekLowerBound(
        const common::BufferView &key) override {
      current_ = key_val_.lower_bound(key);
      return outcome::success();
    }

    outcome::result<void> seekUpperBound(
        const common::BufferView &key) override {
      current_ = key_val_.upper_bound(key);
      return outcome::success();
    }

    outcome::result<bool> seekLast() override {
      current_ = std::prev(key_val_.end());
      return true;
    }

    bool isValid() const override {
      return current_ != key_val_.end();
    }

    outcome::result<void> next() override {
      current_++;
      return outcome::success();
    }

    std::optional<common::Buffer> key() const override {
      return current_->first;
    }

    std::optional<common::BufferConstRef> value() const override {
      return current_->second;
    }
  };
}  // namespace kagome::storage::trie

#endif  // KAGOME_TEST_CORE_STORAGE_TRIE_POLKADOT_TRIE_CURSOR_DUMMY_HPP
