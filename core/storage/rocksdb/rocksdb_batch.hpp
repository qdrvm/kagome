/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>

#include "common/buffer.hpp"
#include "storage/buffer_map_types.hpp"
#include "storage/face/write_batch.hpp"
#include "storage/rocksdb/rocksdb.hpp"

namespace kagome::storage {

  class RocksDbBatch : public BufferSpacedBatch, public BufferBatch {
   public:
    ~RocksDbBatch() override = default;

    explicit RocksDbBatch(std::shared_ptr<RocksDb> db,
                          rocksdb::ColumnFamilyHandle *default_cf);

    outcome::result<void> commit() override;

    void clear() override;

    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override;

    outcome::result<void> put(Space space,
                              const BufferView &key,
                              BufferOrView &&value) override;

    outcome::result<void> remove(const BufferView &key) override;
    outcome::result<void> remove(Space space, const BufferView &key) override;

   private:
    std::shared_ptr<RocksDb> db_;
    rocksdb::WriteBatch batch_;
    rocksdb::ColumnFamilyHandle *default_cf_;
  };
}  // namespace kagome::storage
