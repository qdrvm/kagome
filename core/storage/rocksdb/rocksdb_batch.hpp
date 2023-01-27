/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROCKSDB_BATCH_HPP
#define KAGOME_ROCKSDB_BATCH_HPP

#include <rocksdb/write_batch.h>
#include "storage/rocksdb/rocksdb.hpp"

namespace kagome::storage {

  class RocksDbBatch : public BufferBatch {
   public:
    ~RocksDbBatch() override = default;

    explicit RocksDbBatch(RocksDbSpace &db);

    outcome::result<void> commit() override;

    void clear() override;

    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override;

    outcome::result<void> remove(const BufferView &key) override;

   private:
    RocksDbSpace &db_;
    rocksdb::WriteBatch batch_;
  };
}  // namespace kagome::storage

#endif  // KAGOME_ROCKSDB_BATCH_HPP
