/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb_cursor.hpp"

#include "storage/leveldb/leveldb_util.hpp"

namespace kagome::storage {

  LevelDB::Cursor::Cursor(std::shared_ptr<leveldb::Iterator> it)
      : i_(std::move(it)) {}

  outcome::result<bool> LevelDB::Cursor::seekFirst() {
    i_->SeekToFirst();
    return isValid();
  }

  outcome::result<bool> LevelDB::Cursor::seek(const Buffer &key) {
    i_->Seek(make_slice(key));
    return isValid();
  }

  outcome::result<bool> LevelDB::Cursor::seekLast() {
    i_->SeekToLast();
    return isValid();
  }

  bool LevelDB::Cursor::isValid() const {
    return i_->Valid();
  }

  outcome::result<void> LevelDB::Cursor::next() {
    i_->Next();
    return outcome::success();
  }

  outcome::result<void> LevelDB::Cursor::prev() {
    i_->Prev();
    return outcome::success();
  }

  std::optional<Buffer> LevelDB::Cursor::key() const {
    return isValid() ? std::make_optional(make_buffer(i_->key()))
                     : std::nullopt;
  }

  std::optional<Buffer> LevelDB::Cursor::value() const {
    return isValid() ? std::make_optional(make_buffer(i_->value()))
                     : std::nullopt;
  }

}  // namespace kagome::storage
