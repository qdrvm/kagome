/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/rocksdb/rocksdb_batch.hpp"

#include "storage/database_error.hpp"
#include "storage/rocksdb/rocksdb_util.hpp"

namespace kagome::storage {

  RocksDbBatch::RocksDbBatch(std::shared_ptr<RocksDb> db,
                             rocksdb::ColumnFamilyHandle *default_cf)
      : db_(std::move(db)), default_cf_(default_cf) {
    BOOST_ASSERT(db_ != nullptr);
    BOOST_ASSERT(default_cf_ != nullptr);
  }

  outcome::result<void> RocksDbBatch::put(const BufferView &key,
                                          BufferOrView &&value) {
    return status_as_result(
        batch_.Put(default_cf_, make_slice(key), make_slice(std::move(value))));
  }

  outcome::result<void> RocksDbBatch::put(Space space,
                                          const BufferView &key,
                                          BufferOrView &&value) {
    auto handle = db_->getCFHandle(space);
    return status_as_result(
        batch_.Put(handle, make_slice(key), make_slice(std::move(value))));
  }

  outcome::result<void> RocksDbBatch::remove(const BufferView &key) {
    return status_as_result(batch_.Delete(default_cf_, make_slice(key)));
  }

  outcome::result<void> RocksDbBatch::remove(Space space,
                                             const BufferView &key) {
    auto handle = db_->getCFHandle(space);
    return status_as_result(batch_.Delete(handle, make_slice(key)));
  }

  outcome::result<void> RocksDbBatch::commit() {
    return status_as_result(db_->db_->Write(db_->wo_, &batch_));
  }

  void RocksDbBatch::clear() {
    batch_.Clear();
  }
}  // namespace kagome::storage
