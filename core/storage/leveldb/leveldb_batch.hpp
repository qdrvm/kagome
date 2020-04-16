/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

    outcome::result<void> put(const Buffer &key, const Buffer &value) override;
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;

    outcome::result<void> remove(const Buffer &key) override;

    outcome::result<void> commit() override;

    void clear() override;

   private:
    LevelDB &db_;
    leveldb::WriteBatch batch_;
  };

}  // namespace kagome::storage

#endif  // KAGOME_LEVELDB_BATCH_HPP
