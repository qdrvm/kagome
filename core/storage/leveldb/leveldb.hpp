/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LEVELDB_HPP
#define KAGOME_LEVELDB_HPP

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include "common/logger.hpp"
#include "storage/buffer_map.hpp"

namespace kagome::storage {

  class LevelDB : public PersistedBufferMap {
   public:
    class Batch;
    class Iterator;
    class Logger;

    ~LevelDB() override = default;

    static outcome::result<std::unique_ptr<LevelDB>> create(
        std::string_view path, leveldb::Options options = leveldb::Options(),
        common::Logger logger = common::createLogger("leveldb"));

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

    std::unique_ptr<BufferMapIterator> iterator() override;

    std::unique_ptr<BufferBatch> batch() override;

    outcome::result<Buffer> get(const Buffer &key) const override;

    bool contains(const Buffer &key) const override;

    outcome::result<void> put(const Buffer &key, const Buffer &value) override;

    outcome::result<void> remove(const Buffer &key) override;

   private:
    LevelDB(std::unique_ptr<leveldb::DB> db, common::Logger logger);

    std::unique_ptr<leveldb::DB> db_;
    leveldb::ReadOptions ro_;
    leveldb::WriteOptions wo_;
    common::Logger logger_;
  };

}  // namespace kagome::storage

#endif  // KAGOME_LEVELDB_HPP
