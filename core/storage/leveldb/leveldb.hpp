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

#ifndef KAGOME_LEVELDB_HPP
#define KAGOME_LEVELDB_HPP

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "common/logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::storage {

  /**
   * @brief An implementation of PersistentBufferMap interface, which uses
   * LevelDB as underlying storage.
   */
  class LevelDB : public BufferStorage {
   public:
    class Batch;
    class Cursor;

    ~LevelDB() override = default;

    /**
     * @brief Factory method to create an instance of LevelDB class.
     * @param path filesystem path where database is going to be
     * @param options leveldb options, such as caching, logging, etc.
     * @return instance of LevelDB
     */
    static outcome::result<std::shared_ptr<LevelDB>> create(
        std::string_view path, leveldb::Options options = leveldb::Options());

    /**
     * @brief Set read options, which are used in @see LevelDB#get
     * @param ro options
     */
    void setReadOptions(leveldb::ReadOptions ro);

    /**
     * @brief Set write options, which are used in @see LevelDB#put
     * @param wo options
     */
    void setWriteOptions(leveldb::WriteOptions wo);

    std::unique_ptr<BufferMapCursor> cursor() override;

    std::unique_ptr<BufferBatch> batch() override;

    outcome::result<Buffer> get(const Buffer &key) const override;

    bool contains(const Buffer &key) const override;

    outcome::result<void> put(const Buffer &key, const Buffer &value) override;

    // value will be copied, not moved, due to internal structure of LevelDB
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;

    outcome::result<void> remove(const Buffer &key) override;

   private:
    std::unique_ptr<leveldb::DB> db_;
    leveldb::ReadOptions ro_;
    leveldb::WriteOptions wo_;
    common::Logger logger_;
  };

}  // namespace kagome::storage

#endif  // KAGOME_LEVELDB_HPP
