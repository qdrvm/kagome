/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>

#include "storage/rocksdb/rocksdb.hpp"
#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>

#include "filesystem/common.hpp"

#include "filesystem/directories.hpp"
#include "storage/database_error.hpp"
#include "storage/rocksdb/rocksdb_batch.hpp"
#include "storage/rocksdb/rocksdb_cursor.hpp"
#include "storage/rocksdb/rocksdb_spaces.hpp"
#include "storage/rocksdb/rocksdb_util.hpp"

namespace kagome::storage {
  namespace fs = filesystem;

  RocksDb::RocksDb() : logger_(log::createLogger("RocksDB", "storage")) {
    ro_.fill_cache = false;
  }

  RocksDb::~RocksDb() {
    for (auto *handle : column_family_handles_) {
      db_->DestroyColumnFamilyHandle(handle);
    }
    delete db_;
  }

  outcome::result<std::shared_ptr<RocksDb>> RocksDb::create(
      const filesystem::path &path,
      rocksdb::Options options,
      uint32_t memory_budget_mib,
      bool prevent_destruction) {
    if (!filesystem::createDirectoryRecursive(path)) {
      return DatabaseError::DB_PATH_NOT_CREATED;
    }

    auto log = log::createLogger("RocksDB", "storage");
    auto absolute_path = fs::absolute(path);

    std::error_code ec;
    if (not fs::create_directory(absolute_path.native(), ec) and ec.value()) {
      log->error("Can't create directory {} for database: {}",
                 absolute_path.native(),
                 ec);
      return DatabaseError::IO_ERROR;
    }
    if (not fs::is_directory(absolute_path.native())) {
      log->error("Can't open {} for database: is not a directory",
                 absolute_path.native());
      return DatabaseError::IO_ERROR;
    }

    // calculate state cache size per space
    const auto memory_budget = memory_budget_mib * 1024 * 1024;
    const uint32_t trie_space_cache_size = memory_budget * 0.9;
    const uint32_t other_spaces_cache_size =
        (memory_budget - trie_space_cache_size) / (storage::Space::kTotal - 1);
    std::vector<rocksdb::ColumnFamilyDescriptor> column_family_descriptors;
    for (auto i = 0; i < Space::kTotal; ++i) {
      column_family_descriptors.emplace_back(rocksdb::ColumnFamilyDescriptor{
          spaceName(static_cast<Space>(i)),
          configureColumn(i != Space::kTrieNode ? other_spaces_cache_size
                                                : trie_space_cache_size)});
    }

    std::vector<rocksdb::ColumnFamilyHandle *> column_family_handles;

    std::cout << (int) options.create_missing_column_families << std::endl;
    options.create_missing_column_families = true;
    auto rocks_db = std::shared_ptr<RocksDb>(new RocksDb);
    auto status = rocksdb::DB::Open(options,
                                    path.native(),
                                    column_family_descriptors,
                                    &rocks_db->column_family_handles_,
                                    &rocks_db->db_);
    if (status.ok()) {
      return rocks_db;
    }

    SL_ERROR(log,
             "Can't open database in {}: {}",
             absolute_path.native(),
             status.ToString());

    return status_as_error(status);
  }

  std::shared_ptr<BufferStorage> RocksDb::getSpace(Space space) {
    if (spaces_.contains(space)) {
      return spaces_[space];
    }
    auto space_name = spaceName(space);
    auto column =
        std::find_if(column_family_handles_.begin(),
                     column_family_handles_.end(),
                     [&space_name](const ColumnFamilyHandlePtr &handle) {
                       return handle->GetName() == space_name;
                     });
    if (column_family_handles_.end() == column) {
      throw DatabaseError::INVALID_ARGUMENT;
    }
    auto space_ptr =
        std::make_shared<RocksDbSpace>(weak_from_this(), *column, logger_);
    spaces_[space] = space_ptr;
    return space_ptr;
  }

  void RocksDb::dropColumn(kagome::storage::Space space) {
    auto space_name = spaceName(space);
    auto column_it =
        std::find_if(column_family_handles_.begin(),
                     column_family_handles_.end(),
                     [&space_name](const ColumnFamilyHandlePtr &handle) {
                       return handle->GetName() == space_name;
                     });
    if (column_family_handles_.end() == column_it) {
      throw DatabaseError::INVALID_ARGUMENT;
    }
    auto &handle = *column_it;
    auto e = [this](rocksdb::Status status) {
      if (!status.ok()) {
        logger_->error("DB operation failed: {}", status.ToString());
        throw status_as_error(status);
      }
    };
    e(db_->DropColumnFamily(handle));
    e(db_->DestroyColumnFamilyHandle(handle));
    e(db_->CreateColumnFamily({}, space_name, &handle));
  }

  rocksdb::BlockBasedTableOptions RocksDb::tableOptionsConfiguration(
      uint32_t lru_cache_size_mib, uint32_t block_size_kib) {
    rocksdb::BlockBasedTableOptions table_options;
    table_options.format_version = 5;
    table_options.block_cache = rocksdb::NewLRUCache(
        static_cast<uint64_t>(lru_cache_size_mib * 1024 * 1024));
    table_options.block_size = static_cast<size_t>(block_size_kib * 1024);
    table_options.cache_index_and_filter_blocks = true;
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
    return table_options;
  }

  rocksdb::ColumnFamilyOptions RocksDb::configureColumn(
      uint32_t memory_budget) {
    rocksdb::ColumnFamilyOptions options;
    options.OptimizeLevelStyleCompaction(memory_budget);
    auto table_options = tableOptionsConfiguration(kDefaultLruCacheSizeMiB,
                                                   kDefaultBlockSizeKiB);
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    return options;
  }

  RocksDbSpace::RocksDbSpace(std::weak_ptr<RocksDb> storage,
                             const RocksDb::ColumnFamilyHandlePtr &column,
                             log::Logger logger)
      : storage_{std::move(storage)},
        column_{column},
        logger_{std::move(logger)} {}

  std::unique_ptr<BufferBatch> RocksDbSpace::batch() {
    return std::make_unique<RocksDbBatch>(*this);
  }

  size_t RocksDbSpace::size() const {
    auto rocks = storage_.lock();
    if (!rocks) {
      return 0;
    }
    size_t usage_bytes = 0;
    if (rocks->db_) {
      std::string usage;
      bool result =
          rocks->db_->GetProperty("rocksdb.cur-size-all-mem-tables", &usage);
      if (result) {
        try {
          usage_bytes = std::stoul(usage);
        } catch (...) {
          logger_->error("Unable to parse memory usage value");
        }
      } else {
        logger_->error("Unable to retrieve memory usage value");
      }
    }
    return usage_bytes;
  }

  std::unique_ptr<RocksDbSpace::Cursor> RocksDbSpace::cursor() {
    auto rocks = storage_.lock();
    if (!rocks) {
      throw DatabaseError::STORAGE_GONE;
    }
    auto it = std::unique_ptr<rocksdb::Iterator>(
        rocks->db_->NewIterator(rocks->ro_, column_));
    return std::make_unique<RocksDBCursor>(std::move(it));
  }

  outcome::result<bool> RocksDbSpace::contains(const BufferView &key) const {
    OUTCOME_TRY(rocks, use());
    std::string value;
    auto status = rocks->db_->Get(rocks->ro_, column_, make_slice(key), &value);
    if (status.ok()) {
      return true;
    }

    if (status.IsNotFound()) {
      return false;
    }

    return status_as_error(status);
  }

  bool RocksDbSpace::empty() const {
    auto rocks = storage_.lock();
    if (!rocks) {
      return true;
    }
    auto it = std::unique_ptr<rocksdb::Iterator>(
        rocks->db_->NewIterator(rocks->ro_, column_));
    it->SeekToFirst();
    return it->Valid();
  }

  outcome::result<BufferOrView> RocksDbSpace::get(const BufferView &key) const {
    OUTCOME_TRY(rocks, use());
    std::string value;
    auto status = rocks->db_->Get(rocks->ro_, column_, make_slice(key), &value);
    if (status.ok()) {
      // cannot move string content to a buffer
      return Buffer(
          reinterpret_cast<uint8_t *>(value.data()),                  // NOLINT
          reinterpret_cast<uint8_t *>(value.data()) + value.size());  // NOLINT
    }
    return status_as_error(status);
  }

  outcome::result<std::optional<BufferOrView>> RocksDbSpace::tryGet(
      const BufferView &key) const {
    OUTCOME_TRY(rocks, use());
    std::string value;
    auto status = rocks->db_->Get(rocks->ro_, column_, make_slice(key), &value);
    if (status.ok()) {
      return std::make_optional(Buffer(
          reinterpret_cast<uint8_t *>(value.data()),                   // NOLINT
          reinterpret_cast<uint8_t *>(value.data()) + value.size()));  // NOLINT
    }

    if (status.IsNotFound()) {
      return std::nullopt;
    }

    return status_as_error(status);
  }

  outcome::result<void> RocksDbSpace::put(const BufferView &key,
                                          BufferOrView &&value) {
    OUTCOME_TRY(rocks, use());
    auto status = rocks->db_->Put(
        rocks->wo_, column_, make_slice(key), make_slice(value));
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  outcome::result<void> RocksDbSpace::remove(const BufferView &key) {
    OUTCOME_TRY(rocks, use());
    auto status = rocks->db_->Delete(rocks->wo_, column_, make_slice(key));
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  void RocksDbSpace::compact(const Buffer &first, const Buffer &last) {
    auto rocks = storage_.lock();
    if (!rocks) {
      return;
    }
    if (rocks->db_) {
      std::unique_ptr<rocksdb::Iterator> begin(
          rocks->db_->NewIterator(rocks->ro_, column_));
      first.empty() ? begin->SeekToFirst() : begin->Seek(make_slice(first));
      auto bk = begin->key();
      std::unique_ptr<rocksdb::Iterator> end(
          rocks->db_->NewIterator(rocks->ro_, column_));
      last.empty() ? end->SeekToLast() : end->Seek(make_slice(last));
      auto ek = end->key();
      rocksdb::CompactRangeOptions options;
      rocks->db_->CompactRange(options, column_, &bk, &ek);
    }
  }

  outcome::result<std::shared_ptr<RocksDb>> RocksDbSpace::use() const {
    auto rocks = storage_.lock();
    if (!rocks) {
      return DatabaseError::STORAGE_GONE;
    }
    return rocks;
  }

}  // namespace kagome::storage
