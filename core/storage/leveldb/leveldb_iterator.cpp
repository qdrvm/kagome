/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb_iterator.hpp"

#include <leveldb/iterator.h>
#include "storage/leveldb/leveldb_util.hpp"

namespace kagome::storage {

  LevelDB::Iterator::Iterator(std::shared_ptr<leveldb::Iterator> it)
      : i_(std::move(it)) {}

  void LevelDB::Iterator::seekToFirst() {
    i_->SeekToFirst();
  }

  void LevelDB::Iterator::seek(const Buffer &key) {
    i_->Seek(make_slice(key));
  }

  void LevelDB::Iterator::seekToLast() {
    i_->SeekToLast();
  }

  bool LevelDB::Iterator::isValid() const {
    return i_->Valid();
  }

  void LevelDB::Iterator::next() {
    i_->Next();
  }

  void LevelDB::Iterator::prev() {
    i_->Prev();
  }

  Buffer LevelDB::Iterator::key() const {
    return make_buffer(i_->key());
  }

  Buffer LevelDB::Iterator::value() const {
    return make_buffer(i_->value());
  }

}  // namespace kagome::storage
