/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb_batch.hpp"

#include "storage/leveldb/leveldb_util.hpp"

namespace kagome::storage {

  LevelDB::Batch::Batch(LevelDB &db) : db_(db) {}

  outcome::result<void> LevelDB::Batch::put(const BufferView &key,
                                            const Buffer &value) {
    batch_.Put(make_slice(key), make_slice(value));
    return outcome::success();
  }

  outcome::result<void> LevelDB::Batch::put(const BufferView &key,
                                            Buffer &&value) {
    return put(key, static_cast<const Buffer &>(value));
  }

  outcome::result<void> LevelDB::Batch::remove(const BufferView &key) {
    batch_.Delete(make_slice(key));
    return outcome::success();
  }

  outcome::result<void> LevelDB::Batch::commit() {
    auto status = db_.db_->Write(db_.wo_, &batch_);
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  void LevelDB::Batch::clear() {
    batch_.Clear();
  }

}  // namespace kagome::storage
