/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROCKSDB_HPP
#define KAGOME_ROCKSDB_HPP

#include "storage/buffer_map_types.hpp"

#include <rocksdb/db.h>
#include <boost/container/flat_map.hpp>
#include <boost/filesystem/path.hpp>
#include "log/logger.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::storage {

  class RocksDb : public SpacedStorage,
                  public std::enable_shared_from_this<RocksDb> {
   private:
    using ColumnFamilyHandlePtr = rocksdb::ColumnFamilyHandle *;

   public:
    ~RocksDb() override;

    RocksDb(const RocksDb &) = delete;
    RocksDb(RocksDb &&) noexcept = delete;
    RocksDb &operator=(const RocksDb &) = delete;
    RocksDb &operator=(RocksDb &&) noexcept = delete;

    /**
     * @brief Factory method to create an instance of RocksDb class.
     * @param path filesystem path where database is going to be
     * @param options rocksdb options, such as caching, logging, etc.
     * @param prevent_destruction - avoid destruction of underlying db if true
     * @return instance of RocksDB
     */
    static outcome::result<std::shared_ptr<RocksDb>> create(
        const boost::filesystem::path &path,
        rocksdb::Options options = rocksdb::Options(),
        bool prevent_destruction = false);

    std::shared_ptr<BufferStorage> getSpace(Space space) override;

    /**
     * Implementation specific way to erase the whole space data.
     * Not exposed at SpacedStorage level as only used in pruner.
     * @param space - storage space identifier to clear
     */
    void dropColumn(Space space);

    friend class RocksDbSpace;
    friend class RocksDbBatch;

   private:
    RocksDb();

    rocksdb::DB *db_{};
    std::vector<ColumnFamilyHandlePtr> column_family_handles_;
    boost::container::flat_map<Space, std::shared_ptr<BufferStorage>> spaces_;
    rocksdb::ReadOptions ro_;
    rocksdb::WriteOptions wo_;
    log::Logger logger_;
  };

  class RocksDbSpace : public BufferStorage {
   public:
    ~RocksDbSpace() override = default;

    RocksDbSpace(std::weak_ptr<RocksDb> storage,
                 const RocksDb::ColumnFamilyHandlePtr &column,
                 log::Logger logger);

    std::unique_ptr<BufferBatch> batch() override;

    size_t size() const override;

    std::unique_ptr<Cursor> cursor() override;

    outcome::result<bool> contains(const BufferView &key) const override;

    bool empty() const override;

    outcome::result<BufferOrView> get(const BufferView &key) const override;

    outcome::result<std::optional<BufferOrView>> tryGet(
        const BufferView &key) const override;

    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override;

    outcome::result<void> remove(const BufferView &key) override;

    void compact(const Buffer &first, const Buffer &last);

    friend class RocksDbBatch;

   private:
    // gather storage instance from weak ptr
    outcome::result<std::shared_ptr<RocksDb>> use() const;

    std::weak_ptr<RocksDb> storage_;
    const RocksDb::ColumnFamilyHandlePtr &column_;
    log::Logger logger_;
  };
}  // namespace kagome::storage

#endif  // KAGOME_ROCKSDB_HPP
