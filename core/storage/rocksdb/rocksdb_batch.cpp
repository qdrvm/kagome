/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/rocksdb/rocksdb_batch.hpp"

#include "storage/database_error.hpp"
#include "storage/rocksdb/rocksdb_util.hpp"

namespace kagome::storage {

  RocksDbBatch::RocksDbBatch(RocksDbSpace &db) : db_(db) {}

  outcome::result<void> RocksDbBatch::put(const BufferView &key,
                                          BufferOrView &&value) {
    batch_.Put(db_.column_.handle, make_slice(key), make_slice(value));
    return outcome::success();
  }

  outcome::result<void> RocksDbBatch::remove(const BufferView &key) {
    batch_.Delete(db_.column_.handle, make_slice(key));
    return outcome::success();
  }

  outcome::result<void> RocksDbBatch::commit() {
    auto rocks = db_.storage_.lock();
    if (!rocks) {
      return DatabaseError::IO_ERROR;
    }
    auto status = rocks->db_->Write(rocks->wo_, &batch_);
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  void RocksDbBatch::clear() {
    batch_.Clear();
  }
}  // namespace kagome::storage
