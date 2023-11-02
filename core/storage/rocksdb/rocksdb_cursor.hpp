/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <rocksdb/iterator.h>
#include "storage/rocksdb/rocksdb.hpp"

namespace kagome::storage {

  class RocksDBCursor : public BufferStorageCursor {
   public:
    ~RocksDBCursor() override = default;

    explicit RocksDBCursor(std::shared_ptr<rocksdb::Iterator> it);

    outcome::result<bool> seekFirst() override;

    outcome::result<bool> seek(const BufferView &key) override;

    outcome::result<bool> seekLast() override;

    bool isValid() const override;

    outcome::result<void> next() override;

    outcome::result<void> prev() override;

    std::optional<Buffer> key() const override;

    std::optional<BufferOrView> value() const override;

   private:
    std::shared_ptr<rocksdb::Iterator> i_;
  };

}  // namespace kagome::storage
