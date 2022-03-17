/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LEVELDB_BATCH_HPP
#define KAGOME_LEVELDB_BATCH_HPP

#include <leveldb/write_batch.h>
#include "storage/leveldb/leveldb.hpp"

namespace kagome::storage {

  /**
   * @brief Class that is used to implement efficient bulk (batch) modifications
   * of the Map.
   */
  class LevelDB::Batch : public BufferBatch {
   public:
    explicit Batch(LevelDB &db);

    outcome::result<void> put(const BufferView &key,
                              const Buffer &value) override;
    outcome::result<void> put(const BufferView &key, Buffer &&value) override;

    outcome::result<void> remove(const BufferView &key) override;

    outcome::result<void> commit() override;

    void clear() override;

   private:
    LevelDB &db_;
    leveldb::WriteBatch batch_;
  };

}  // namespace kagome::storage

#endif  // KAGOME_LEVELDB_BATCH_HPP
