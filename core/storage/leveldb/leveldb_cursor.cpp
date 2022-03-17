/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb_cursor.hpp"

#include "storage/leveldb/leveldb_util.hpp"

namespace kagome::storage {

  LevelDBCursor::LevelDBCursor(std::shared_ptr<leveldb::Iterator> it)
      : i_(std::move(it)) {}

  outcome::result<bool> LevelDBCursor::seekFirst() {
    i_->SeekToFirst();
    return isValid();
  }

  outcome::result<bool> LevelDBCursor::seek(const BufferView &key) {
    i_->Seek(make_slice(key));
    return isValid();
  }

  outcome::result<bool> LevelDBCursor::seekLast() {
    i_->SeekToLast();
    return isValid();
  }

  bool LevelDBCursor::isValid() const {
    return i_->Valid();
  }

  outcome::result<void> LevelDBCursor::next() {
    i_->Next();
    return outcome::success();
  }

  outcome::result<void> LevelDBCursor::prev() {
    i_->Prev();
    return outcome::success();
  }

  std::optional<Buffer> LevelDBCursor::key() const {
    return isValid() ? std::make_optional(make_buffer(i_->key()))
                     : std::nullopt;
  }

  std::optional<Buffer> LevelDBCursor::value() const {
    return isValid() ? std::make_optional(make_buffer(i_->value()))
                     : std::nullopt;
  }

}  // namespace kagome::storage
