/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/rocksdb/rocksdb_batch.hpp"

#include "storage/database_error.hpp"
#include "storage/rocksdb/rocksdb_util.hpp"

namespace kagome::storage {

  RocksDB::Batch::Batch(RocksDB &db) : db_(db) {}

  outcome::result<void> RocksDB::Batch::put(const BufferView &key,
                                            BufferOrView &&value) {
    batch_.Put(make_slice(key), make_slice(value));
    return outcome::success();
  }

  outcome::result<void> RocksDB::Batch::remove(const BufferView &key) {
    batch_.Delete(make_slice(key));
    return outcome::success();
  }

  outcome::result<void> RocksDB::Batch::commit() {
    auto status = db_.db_->Write(db_.wo_, &batch_);
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  void RocksDB::Batch::clear() {
    batch_.Clear();
  }
}  // namespace kagome::storage
