/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROCKSDB_BATCH_HPP
#define KAGOME_ROCKSDB_BATCH_HPP

#include <rocksdb/write_batch.h>
#include "storage/rocksdb/rocksdb.hpp"

namespace kagome::storage {

  class RocksDB::Batch : public BufferBatch {
   public:
    ~Batch() override = default;

    explicit Batch(RocksDB &db);

    outcome::result<void> commit() override;

    void clear() override;

    outcome::result<void> put(const BufferView &key,
                              const Buffer &value) override;

    outcome::result<void> put(const BufferView &key, Buffer &&value) override;

    outcome::result<void> remove(const BufferView &key) override;

   private:
    RocksDB &db_;
    rocksdb::WriteBatch batch_;
  };
}  // namespace kagome::storage

#endif  // KAGOME_ROCKSDB_BATCH_HPP
