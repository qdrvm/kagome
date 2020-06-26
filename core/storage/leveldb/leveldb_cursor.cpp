/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb_cursor.hpp"

#include "storage/leveldb/leveldb_util.hpp"

namespace kagome::storage {

  LevelDB::Cursor::Cursor(std::shared_ptr<leveldb::Iterator> it)
      : i_(std::move(it)) {}

  outcome::result<void> LevelDB::Cursor::seekToFirst() {
    i_->SeekToFirst();
    return outcome::success();
  }

  outcome::result<void> LevelDB::Cursor::seek(const Buffer &key) {
    i_->Seek(make_slice(key));
    return outcome::success();
  }

  outcome::result<void> LevelDB::Cursor::seekToLast() {
    i_->SeekToLast();
    return outcome::success();
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

  outcome::result<Buffer> LevelDB::Cursor::key() const {
    return make_buffer(i_->key());
  }

  outcome::result<Buffer> LevelDB::Cursor::value() const {
    return make_buffer(i_->value());
  }

}  // namespace kagome::storage
