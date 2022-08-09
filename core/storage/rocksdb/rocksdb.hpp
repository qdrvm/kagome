/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROCKSDB_HPP
#define KAGOME_ROCKSDB_HPP

#include "storage/buffer_map_types.hpp"

#include <rocksdb/db.h>
#include <boost/filesystem/path.hpp>
#include "log/logger.hpp"

namespace kagome::storage {

  class RocksDB : public BufferStorage {
   public:
    class Batch;

    ~RocksDB() override = default;

    /**
     * @brief Factory method to create an instance of RocksDB class.
     * @param path filesystem path where database is going to be
     * @param options rocksdb options, such as caching, logging, etc.
     * @return instance of RocksDB
     */
    static outcome::result<std::unique_ptr<RocksDB>> create(
        const boost::filesystem::path &path,
        rocksdb::Options options = rocksdb::Options());

    std::unique_ptr<BufferBatch> batch() override;

    size_t size() const override;

    std::unique_ptr<Cursor> cursor() override;

    outcome::result<bool> contains(const Key &key) const override;

    bool empty() const override;

    outcome::result<kagome::storage::Buffer> load(
        const Key &key) const override;

    outcome::result<std::optional<Buffer>> tryLoad(
        const Key &key) const override;

    outcome::result<void> put(const BufferView &key,
                              const Buffer &value) override;

    outcome::result<void> put(const BufferView &key, Buffer &&value) override;

    outcome::result<void> remove(const BufferView &key) override;

    void compact(const Buffer &first, const Buffer &last);

   private:
    RocksDB();

    std::unique_ptr<rocksdb::DB> db_;
    rocksdb::ReadOptions ro_;
    rocksdb::WriteOptions wo_;
    log::Logger logger_;
  };
}  // namespace kagome::storage

#endif  // KAGOME_ROCKSDB_HPP
